/*
 * Copyright 2007-2019 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2019
 * Copyright Siemens AG 2015-2019
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* CMP functions for PKIMessage checking */

#include "cmp_local.h"
#include <openssl/cmp_util.h>

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/asn1t.h>
#include <openssl/cmp.h>
#include <openssl/crmf.h>
#include <openssl/err.h>
#include <openssl/x509.h>

static void add_mem_bio_to_error_line(BIO *bio)
{
    if (bio != NULL) {
        char *str;
        long len = BIO_get_mem_data(bio, &str);

        if (len > 0) {
            str[len - 1] = '\0'; /* replace last '\n', terminating str */
            ossl_cmp_add_error_line(str);
        }
    }
}

static void print_cert(BIO *bio, X509 *cert, unsigned long neg_cflags);
/*
 * Verify a message protected by signature according to section 5.1.3.3
 * (sha1+RSA/DSA or any other algorithm supported by OpenSSL).
 *
 * Returns 1 on successful validation and 0 otherwise.
 */
static int verify_signature(const OSSL_CMP_CTX *cmp_ctx,
                            const OSSL_CMP_MSG *msg, X509 *cert)
{
    EVP_MD_CTX *ctx = NULL;
    CMP_PROTECTEDPART prot_part;
    int digest_nid, pk_nid;
    EVP_MD *digest = NULL;
    EVP_PKEY *pubkey = NULL;
    int l;
    size_t prot_part_der_len = 0;
    unsigned char *prot_part_der = NULL;
    BIO *bio = BIO_new(BIO_s_mem()); /* may be NULL */
    int res = 0;

    if (!ossl_assert(cmp_ctx != NULL && msg != NULL && cert != NULL))
        return 0;

    /* verify that keyUsage, if present, contains digitalSignature */
    if (!cmp_ctx->ignore_keyusage
            && (X509_get_key_usage(cert) & X509v3_KU_DIGITAL_SIGNATURE) == 0) {
        CMPerr(0, CMP_R_MISSING_KEY_USAGE_DIGITALSIGNATURE);
        goto sig_err;
    }

    pubkey = X509_get_pubkey(cert);
    if (pubkey == NULL) {
        CMPerr(0, CMP_R_FAILED_EXTRACTING_PUBKEY);
        goto sig_err;
    }

    /* create the DER representation of protected part */
    prot_part.header = msg->header;
    prot_part.body = msg->body;

    l = i2d_CMP_PROTECTEDPART(&prot_part, &prot_part_der);
    if (l < 0 || prot_part_der == NULL)
        goto end;
    prot_part_der_len = (size_t) l;

    /* verify signature of protected part */
    if (!OBJ_find_sigid_algs(OBJ_obj2nid(msg->header->protectionAlg->algorithm),
                             &digest_nid, &pk_nid)
            || digest_nid == NID_undef || pk_nid == NID_undef
            || (digest = (EVP_MD *)EVP_get_digestbynid(digest_nid)) == NULL) {
        CMPerr(0, CMP_R_ALGORITHM_NOT_SUPPORTED);
        goto sig_err;
    }

    /* check msg->header->protectionAlg is consistent with public key type */
    if (EVP_PKEY_type(pk_nid) != EVP_PKEY_base_id(pubkey)) {
        CMPerr(0, CMP_R_WRONG_ALGORITHM_OID);
        goto sig_err;
    }
    if ((ctx = EVP_MD_CTX_new()) == NULL)
        goto end;
    if (EVP_DigestVerifyInit(ctx, NULL, digest, NULL, pubkey)
            && EVP_DigestVerify(ctx, msg->protection->data,
                                msg->protection->length,
                                prot_part_der, prot_part_der_len) == 1) {
        res = 1;
        goto end;
    }

 sig_err:
    print_cert(bio, cert, X509_FLAG_NO_EXTENSIONS);
    CMPerr(0, CMP_R_ERROR_VALIDATING_PROTECTION);
    add_mem_bio_to_error_line(bio);

 end:
    EVP_MD_CTX_free(ctx);
    OPENSSL_free(prot_part_der);
    EVP_PKEY_free(pubkey);
    BIO_free(bio);

    return res;
}

/* Verify a message protected with PBMAC */
static int CMP_verify_PBMAC(const OSSL_CMP_MSG *msg,
                            const ASN1_OCTET_STRING *secret)
{
    ASN1_BIT_STRING *protection = NULL;
    int valid = 0;

    /* generate expected protection for the message */
    if ((protection = ossl_cmp_calc_protection(msg, secret, NULL)) == NULL)
        return 0;               /* failed to generate protection string! */

    valid = (msg->protection != NULL && msg->protection->length >= 0)
        && (msg->protection->type == protection->type)
        && (msg->protection->length == protection->length)
        && (CRYPTO_memcmp(msg->protection->data, protection->data,
                          protection->length) == 0);
    ASN1_BIT_STRING_free(protection);
    if (!valid)
        CMPerr(0, CMP_R_WRONG_PBM_VALUE);

    return valid;
}

/*
 * Attempt to validate certificate and path using any given store with trusted
 * certs (possibly including CRLs and a cert verification callback function)
 * and non-trusted intermediate certs from the given ctx.
 *
 * Returns 1 on successful validation and 0 otherwise.
 */
int OSSL_CMP_validate_cert_path(OSSL_CMP_CTX *ctx, X509_STORE *trusted_store,
                                X509 *cert)
{
    int valid = 0;
    X509_STORE_CTX *csc = NULL;
    const int err = CMP_R_POTENTIALLY_INVALID_CERTIFICATE;

    if (!ossl_assert(ctx != NULL && cert != NULL))
        return 0;

    if (trusted_store == NULL) {
        CMPerr(0, CMP_R_MISSING_TRUST_STORE);
        return 0;
    }

    if ((csc = X509_STORE_CTX_new()) == NULL
            || !X509_STORE_CTX_init(csc, trusted_store,
                                    cert, ctx->untrusted_certs))
        goto err;

    valid = X509_verify_cert(csc) > 0;

    /* make sure suitable error is queued even if callback did not do */
    if (!valid && ERR_GET_REASON(ERR_peek_last_error()) != err)
        CMPerr(0, err);

 err:
    X509_STORE_CTX_free(csc);
    return valid;
}

/* Helper functions for improving certificate verification error diagnostics */
static void print_cert(BIO *bio, X509 *cert, unsigned long neg_cflags)
{
    if (cert != NULL) {
        unsigned long flags = ASN1_STRFLGS_RFC2253 | ASN1_STRFLGS_ESC_QUOTE |
            XN_FLAG_SEP_CPLUS_SPC | XN_FLAG_FN_SN;

        BIO_printf(bio, "    certificate\n");
        X509_print_ex(bio, cert, flags, ~X509_FLAG_NO_SUBJECT);
        if (X509_check_issued((X509 *)cert, cert) == X509_V_OK) {
            BIO_printf(bio, "        self-signed\n");
        } else {
            BIO_printf(bio, " ");
            X509_print_ex(bio, cert, flags, ~X509_FLAG_NO_ISSUER);
        }
        X509_print_ex(bio, cert, flags,
                      ~(X509_FLAG_NO_SERIAL | X509_FLAG_NO_VALIDITY));
        if (X509_cmp_current_time(X509_get0_notBefore(cert)) > 0)
            BIO_printf(bio, "        not yet valid\n");

        if (X509_cmp_current_time(X509_get0_notAfter(cert)) < 0)
            BIO_printf(bio, "        no more valid\n");

        X509_print_ex(bio, cert, flags, ~(neg_cflags));
    } else {
        BIO_printf(bio, "    (no certificate)\n");
    }
}

static void print_certs(BIO *bio, const STACK_OF(X509) *certs)
{
    if (certs != NULL && sk_X509_num(certs) > 0) {
        int i;

        for (i = 0; i < sk_X509_num(certs); i++) {
            X509 *cert = sk_X509_value(certs, i);
            if (cert != NULL)
                print_cert(bio, cert, 0);
        }
    } else {
        BIO_printf(bio, "    (no certificates)\n");
    }
}

static void print_store_certs(BIO *bio, X509_STORE *store)
{
    if (store != NULL) {
        STACK_OF(X509) *certs = ossl_cmp_X509_STORE_get1_certs(store);
        print_certs(bio, certs);
        sk_X509_pop_free(certs, X509_free);
    } else {
        BIO_printf(bio, "    (no certificate store)\n");
    }
}

/*
 * Diagnostic function that may be registered using
 * X509_STORE_set_verify_cb(), such that it gets called by OpenSSL's
 * verify_cert() function at the end of a cert verification as an opportunity
 * to gather and queue information regarding a (failing) cert verification,
 * and to possibly change the result of the verification (not done here).
 * The CLI also calls it on error while cert status checking using OCSP stapling
 * via a callback function set with SSL_CTX_set_tlsext_status_cb().
 *
 * Returns 0 if and only if the cert verification is considered failed.
 */
int OSSL_CMP_print_cert_verify_cb(int ok, X509_STORE_CTX *ctx)
{
    if (ok == 0 && ctx != NULL) {
        int cert_error = X509_STORE_CTX_get_error(ctx);
        int depth = X509_STORE_CTX_get_error_depth(ctx);
        X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
        BIO *bio = BIO_new(BIO_s_mem()); /* may be NULL */

        BIO_printf(bio, "%s at depth=%d error=%d (%s)\n",
                   X509_STORE_CTX_get0_parent_ctx(ctx) != NULL
                   ? "CRL path validation" : "certificate verification",
                   depth, cert_error,
                   X509_verify_cert_error_string(cert_error));
        BIO_printf(bio, "failure for:\n");
        print_cert(bio, cert, X509_FLAG_NO_EXTENSIONS);
        if (cert_error == X509_V_ERR_CERT_UNTRUSTED
                || cert_error == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
                || cert_error == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN
                || cert_error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT
                || cert_error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY
                || cert_error == X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER
                || cert_error == X509_V_ERR_STORE_LOOKUP) {
            BIO_printf(bio, "non-trusted certs:\n");
            print_certs(bio, X509_STORE_CTX_get0_untrusted(ctx));
            BIO_printf(bio, "trust store:\n");
            print_store_certs(bio, X509_STORE_CTX_get0_store(ctx));
        }
        CMPerr(0, CMP_R_POTENTIALLY_INVALID_CERTIFICATE);
        add_mem_bio_to_error_line(bio);
        BIO_free(bio);
    }

    /*
     * TODO we could check policies here too, e.g.:
     * if (cert_error == X509_V_OK && ok == 2)
     *     policies_print(NULL, ctx);
     */

    return ok;
}

/*
 * Return 0 if time should not be checked or reference time is within frame,
 * or else 1 if it s past the end, or -1 if it is before the start
 */
/* TODO remove definition when respective PR has been merged */
int OSSL_CMP_cmp_timeframe(const ASN1_TIME *start,
                           const ASN1_TIME *end, X509_VERIFY_PARAM *vpm)
{
    time_t check_time, *ptime = NULL;
    unsigned long flags = vpm == NULL ? 0 : X509_VERIFY_PARAM_get_flags(vpm);

    if ((flags & X509_V_FLAG_USE_CHECK_TIME) != 0) {
        check_time = X509_VERIFY_PARAM_get_time(vpm);
        ptime = &check_time;
    } else if ((flags & X509_V_FLAG_NO_CHECK_TIME) != 0) {
        return 0; /* ok */
    }
    if (end != NULL && X509_cmp_time(end, ptime) < 0)
        return 1;
    if (start != NULL && X509_cmp_time(start, ptime) > 0)
        return -1;
    return 0;
}

/* Return 0 if expect_name != NULL and there is no matching actual_name */
static int check_name(OSSL_CMP_CTX *ctx,
                      const char *actual_desc, const X509_NAME *actual_name,
                      const char *expect_desc, const X509_NAME *expect_name)
{
    char *actual, *expect;

    if (expect_name == NULL)
        return 1; /* no expectation, thus trivially fulfilled */

    /* make sure that a matching name is there */
    if (actual_name == NULL) {
        OSSL_CMP_log1(WARN, ctx, " missing %s", actual_desc);
        return 0;
    }
    if (X509_NAME_cmp(actual_name, expect_name) == 0)
        return 1;

    actual = X509_NAME_oneline(actual_name, NULL, 0);
    expect = X509_NAME_oneline(expect_name, NULL, 0);
    OSSL_CMP_log2(INFO, ctx, " actual name in %s = %s", actual_desc, actual);
    OSSL_CMP_log2(INFO, ctx, " does not match %s = %s", expect_desc, expect);
    OPENSSL_free(expect);
    OPENSSL_free(actual);
    return 0;
}

/* Return 0 if skid != NULL and there is no matching subject key ID in cert */
static int check_kid(OSSL_CMP_CTX *ctx,
                     X509 *cert, const ASN1_OCTET_STRING *skid)
{
    char *actual, *expect;
    const ASN1_OCTET_STRING *ckid = X509_get0_subject_key_id(cert);

    if (skid == NULL)
        return 1; /* no expectation, thus trivially fulfilled */

    /* make sure that the expected subject key identifier is there */
    if (ckid == NULL) {
        OSSL_CMP_warn(ctx, " missing Subject Key Identifier in certificate");
        return 0;
    }
    if (ASN1_OCTET_STRING_cmp(ckid, skid) == 0)
        return 1;

    if ((actual = OPENSSL_buf2hexstr(ckid->data, ckid->length)) != NULL)
        OSSL_CMP_log1(INFO, ctx, " cert Subject Key Identifier = %s", actual);
    if ((expect = OPENSSL_buf2hexstr(skid->data, skid->length)) != NULL)
        OSSL_CMP_log1(INFO, ctx, " does not match senderKID    = %s", expect);
    OPENSSL_free(expect);
    OPENSSL_free(actual);
    return 0;
}

/*
 * Check if the given cert is acceptable as sender cert of the given message.
 * The subject DN must match, the subject key ID as well if present in the msg,
 * and the cert must be current (checked if ctx->trusted is not NULL).
 * Note that cert revocation etc. is checked by OSSL_CMP_validate_cert_path().
 *
 * Returns 0 on error or not acceptable, else 1.
 */
static int cert_acceptable(OSSL_CMP_CTX *ctx, const char *desc, X509 *cert,
                           const STACK_OF(X509) *already_checked,
                           const OSSL_CMP_MSG *msg)
{
    X509_STORE *ts = ctx->trusted;
    char *sub, *iss;
    X509_VERIFY_PARAM *vpm;
    int time_cmp;
    int i;

    OSSL_CMP_log1(INFO, ctx, " considering %s with..", desc);
    if ((sub = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0)) != NULL)
        OSSL_CMP_log1(INFO, ctx, "  subject = %s", sub);
    if ((iss = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0)) != NULL)
        OSSL_CMP_log1(INFO, ctx, "  issuer  = %s", iss);
    OPENSSL_free(iss);
    OPENSSL_free(sub);

    for (i = sk_X509_num(already_checked /* may be NULL */); i > 0; i--)
        if (X509_cmp(sk_X509_value(already_checked, i - 1), cert) == 0) {
            OSSL_CMP_info(ctx, " cert has already been checked");
            return 0;
        }

    vpm = ts != NULL ? X509_STORE_get0_param(ts) : NULL;
    if (!ossl_assert(cert != NULL && msg != NULL && (ts == NULL || vpm != NULL)))
        return 0;

    time_cmp = OSSL_CMP_cmp_timeframe(X509_get0_notBefore(cert),
                                      X509_get0_notAfter(cert), vpm);
    if (time_cmp != 0) {
        OSSL_CMP_warn(ctx, time_cmp > 0 ? " cert has expired"
                                        : " cert is not yet valid");
        return 0;
    }

    if (!check_name(ctx,
                    "cert subject", X509_get_subject_name(cert),
                    "sender field", msg->header->sender->d.directoryName))
        return 0;

    if (!check_kid(ctx, cert, msg->header->senderKID))
        return 0;
    /* acceptable also if there is no senderKID in msg header */
    OSSL_CMP_info(ctx, " cert is acceptable");
    return 1;
}

static int check_msg_valid_cert(OSSL_CMP_CTX *ctx, X509_STORE *store,
                          X509 *scrt, const OSSL_CMP_MSG *msg)
{
    return verify_signature(ctx, msg, scrt)
        && OSSL_CMP_validate_cert_path(ctx, store, scrt);
}

/*
 * Exceptional handling for 3GPP TS 33.310 [3G/LTE Network Domain Security
 * (NDS); Authentication Framework (AF)], only to use for IP and if the ctx
 * option is explicitly set: use self-signed certificates from extraCerts as
 * trust anchor to validate sender cert and msg -
 * provided it also can validate the newly enrolled certificate
 */
static int check_msg_valid_cert_3gpp(OSSL_CMP_CTX *ctx, X509 *scrt,
                                     const OSSL_CMP_MSG *msg)
{
    int valid = 0;
    X509_STORE *store = X509_STORE_new();

    if (store != NULL /* store does not include CRLs */
            && ossl_cmp_X509_STORE_add1_certs(store, msg->extraCerts,
                                              1 /* self-signed only */))
        valid = check_msg_valid_cert(ctx, store, scrt, msg);
    if (valid) {
        /*
         * verify that the newly enrolled certificate (which is assumed to have
         * rid == 0) can also be validated with the same trusted store
         */
        EVP_PKEY *privkey = OSSL_CMP_CTX_get0_newPkey(ctx, 1);
        OSSL_CMP_CERTRESPONSE *crep =
            ossl_cmp_certrepmessage_get0_certresponse(msg->body->value.ip, 0);
        X509 *newcrt = ossl_cmp_certresponse_get1_certificate(privkey, crep);
        /*
         * maybe better use get_cert_status() from cmp_client.c, which catches
         * errors
         */
        valid = OSSL_CMP_validate_cert_path(ctx, store, newcrt);
        X509_free(newcrt);
    }
    X509_STORE_free(store);
    return valid;
}

/*
 * Try all certs in given list for verifying msg, normally or in 3GPP mode.
 * If already_checked==NULL then certs are assumed to contain extraCerts first.
 */
static int check_msg_with_certs(OSSL_CMP_CTX *ctx, STACK_OF(X509) *certs,
                                const STACK_OF(X509) *already_checked,
                                const OSSL_CMP_MSG *msg, int mode_3gpp)
{
    int i, n_acceptable_extraCerts = 0;

    for (i = 0; i < sk_X509_num(certs); i++) { /* certs may be NULL */
        X509 *cert = sk_X509_value(certs, i);
        int in_extraCerts = already_checked == NULL &&
            X509_find_by_issuer_and_serial(msg->extraCerts,
                                           X509_get_issuer_name(cert),
                                           (ASN1_INTEGER *)
                                           X509_get0_serialNumber(cert));
        if (!mode_3gpp && already_checked == NULL && !in_extraCerts
                && n_acceptable_extraCerts == 0) {
                OSSL_CMP_warn(ctx, "no acceptable cert in extraCerts");
                n_acceptable_extraCerts++; /* thus warning is shown only once */
        }
        if (!cert_acceptable(ctx,
                             already_checked != 0 ? "cert from trusted store" :
                             in_extraCerts ? "cert from extraCerts"
                                           : "cert from other untrusted source",
                             cert, already_checked, msg))
            continue;
        if (in_extraCerts)
            n_acceptable_extraCerts++;
        if (mode_3gpp ? check_msg_valid_cert_3gpp(ctx, cert, msg)
                      : check_msg_valid_cert(ctx, ctx->trusted, cert, msg)) {
            /* store successfull sender cert for further msgs in transaction */
            if (X509_up_ref(cert)
                    && !ossl_cmp_ctx_set0_validatedSrvCert(ctx, cert))
                X509_free(cert);
            return 1;
        }
    }
    return 0;
}

/*
 * Verify msg trying first ctx->untrusted_certs, which should include extraCerts
 * at its front, then trying the trusted certs in truststore (if any) of ctx.
 */
static int check_msg_all_certs(OSSL_CMP_CTX *ctx, const OSSL_CMP_MSG *msg,
                               int mode_3gpp)
{
    int ret = 0;

    OSSL_CMP_info(ctx,
                  mode_3gpp ? "failed; trying now 3GPP mode trusting extraCerts"
                            : "trying first normal mode using trust store");
    if (check_msg_with_certs(ctx, ctx->untrusted_certs, NULL, msg, mode_3gpp))
        return 1;

    if (ctx->trusted != NULL) {
        STACK_OF(X509) *trusted = ossl_cmp_X509_STORE_get1_certs(ctx->trusted);
        ret = check_msg_with_certs(ctx, trusted, ctx->untrusted_certs,
                                   msg, mode_3gpp);
        sk_X509_pop_free(trusted, X509_free);
    }
    return ret;
}

/* verify message signature with any acceptable and valid candidate cert */
static int check_msg_find_cert(OSSL_CMP_CTX *ctx, const OSSL_CMP_MSG *msg)
{
    X509 *scrt = ctx->validatedSrvCert; /* previous successful sender cert */
    GENERAL_NAME *sender = msg->header->sender;
    char *sname;
    const ASN1_OCTET_STRING *skid = msg->header->senderKID;
    char *skid_str;
    int res = 0;

    if (sender == NULL || msg->body == NULL)
        return 0; /* other NULL cases already have been checked */
    if (sender->type != GEN_DIRNAME) {
        CMPerr(0, CMP_R_SENDER_GENERALNAME_TYPE_NOT_SUPPORTED);
        return 0;
    }

    /*
     * try first cached scrt, used successfully earlier in same transaction,
     * for validating this and any further msgs where extraCerts may be left out
     */
    (void)ERR_set_mark();
    if (scrt != NULL
            && cert_acceptable(ctx, "previously validated sender cert",
                               scrt, NULL, msg)
            && (check_msg_valid_cert(ctx, ctx->trusted, scrt, msg)
                    || check_msg_valid_cert_3gpp(ctx, scrt, msg))) {
        (void)ERR_pop_to_mark();
        return 1;
    }
    (void)ERR_pop_to_mark();

    /* release any cached sender cert that proved no more successfully usable */
    (void)ossl_cmp_ctx_set0_validatedSrvCert(ctx, NULL);

    /* start accumulating diagnostic info via CMP logging */
    OSSL_CMP_info(ctx, "trying to verify msg signature with valid cert that..");
    sname = X509_NAME_oneline(sender->d.directoryName, NULL, 0);
    if (sname != NULL)
        OSSL_CMP_log1(INFO, ctx, "matches msg sender name = %s", sname);
    skid_str = skid == NULL ? NULL
                            : OPENSSL_buf2hexstr(skid->data, skid->length);
    if (skid_str != NULL)
        OSSL_CMP_log1(INFO, ctx, "matches msg senderKID   = %s", skid_str);
    else
        OSSL_CMP_info(ctx, "while msg header does not contain senderKID");

    /* enable clearing irrelevant errors in attempts to validate sender certs */
    (void)ERR_set_mark();

    if (check_msg_all_certs(ctx, msg, 0 /* using ctx->trusted */)
            || check_msg_all_certs(ctx, msg, 1 /* 3gpp */)) {
        /* discard any diagnostic info on trying to use certs */
        (void)ERR_pop_to_mark();
        res = 1;
        goto end;
    }
    (void)ERR_clear_last_mark();

    CMPerr(0, CMP_R_NO_SUITABLE_SENDER_CERT);
    if (sname != NULL) {
        ossl_cmp_add_error_txt(NULL, "for msg sender name = ");
        ossl_cmp_add_error_txt(NULL, sname);
    }
    if (skid_str != NULL) {
        ossl_cmp_add_error_txt(" and ", "for msg senderKID = ");
        ossl_cmp_add_error_txt(NULL, skid_str);
    }

 end:
    OPENSSL_free(sname);
    OPENSSL_free(skid_str);
    return res;
}

/*
 * Validate the protection of the given PKIMessage using either password-
 * based mac (PBM) or a signature algorithm. In the case of signature algorithm,
 * the sender certificate can have been pinned by providing it in ctx->srvCert,
 * else it is taken from ctx->untrusted_certs (which should include extraCerts
 * first) and from ctx->trusted and validated against ctx->trusted.
 *
 * If ctx->permitTAInExtraCertsForIR is true, the trust anchor may be taken from
 * the extraCerts field when a self-signed certificate is found there which can
 * be used to validate the enrolled certificate returned in IP.
 * This is according to the need given in 3GPP TS 33.310.
 *
 * Returns 1 on success, 0 on error or validation failed.
 */
int OSSL_CMP_validate_msg(OSSL_CMP_CTX *ctx, const OSSL_CMP_MSG *msg)
{
    X509_ALGOR *alg;
    int nid = NID_undef, pk_nid = NID_undef;
    const ASN1_OBJECT *algorOID = NULL;
    X509 *scrt;

    if (!ossl_assert(ctx != NULL && msg != NULL && msg->header != NULL))
        return 0;

    if ((alg = msg->header->protectionAlg) == NULL /* unprotected message */
            || msg->protection == NULL || msg->protection->data == NULL) {
        CMPerr(0, CMP_R_MISSING_PROTECTION);
        return 0;
    }

    /* determine the nid for the used protection algorithm */
    X509_ALGOR_get0(&algorOID, NULL, NULL, alg);
    nid = OBJ_obj2nid(algorOID);

    switch (nid) {
        /* 5.1.3.1.  Shared Secret Information */
    case NID_id_PasswordBasedMAC:
        if (CMP_verify_PBMAC(msg, ctx->secretValue)) {
            /*
             * RFC 4210, 5.3.2: 'Note that if the PKI Message Protection is
             * "shared secret information", then any certificate transported in
             * the caPubs field may be directly trusted as a root CA
             * certificate by the initiator.'
             */
            switch (ossl_cmp_msg_get_bodytype(msg)) {
            case OSSL_CMP_PKIBODY_IP:
            case OSSL_CMP_PKIBODY_CP:
            case OSSL_CMP_PKIBODY_KUP:
            case OSSL_CMP_PKIBODY_CCP:
                if (ctx->trusted != NULL) {
                    STACK_OF(X509) *certs = msg->body->value.ip->caPubs;
                    /* value.ip is same for cp, kup, and ccp */

                    if (!ossl_cmp_X509_STORE_add1_certs(ctx->trusted, certs, 0))
                        /* adds both self-signed and not self-signed certs */
                        break;
                }
            }
            return 1;
        }
        break;

        /*
         * 5.1.3.2 DH Key Pairs
         * Not yet supported
         */
    case NID_id_DHBasedMac:
        CMPerr(0, CMP_R_UNSUPPORTED_PROTECTION_ALG_DHBASEDMAC);
        break;

        /*
         * 5.1.3.3.  Signature
         */
    default:
        if (!OBJ_find_sigid_algs(OBJ_obj2nid(alg->algorithm), NULL, &pk_nid)
                || pk_nid == NID_undef) {
            CMPerr(0, CMP_R_UNKNOWN_ALGORITHM_ID);
            break;
        }
        /* validate sender name of received msg */
        if (msg->header->sender->type != GEN_DIRNAME) {
            CMPerr(0, CMP_R_SENDER_GENERALNAME_TYPE_NOT_SUPPORTED);
            break; /* FR#42: support for more than X509_NAME */
        }
        /*
         * Compare actual sender name of response with expected sender name.
         * Expected name can be set explicitly or the subject of ctx->srvCert.
         * Mitigates risk to accept misused certificate of an unauthorized
         * entity of a trusted hierarchy.
         */
        if (!check_name(ctx, "sender DN field",
                        msg->header->sender->d.directoryName,
                        "expected sender", ctx->expected_sender))
            break;
        /* Note: if recipient was NULL-DN it could be learned here if needed */

        scrt = ctx->srvCert;
        if (scrt == NULL) {
            if (check_msg_find_cert(ctx, msg))
                return 1;
        } else { /* use pinned sender cert */
            /*
             * We try using ctx->srvCert for sig check even if not acceptable.
             * cert_acceptable() is called here just to add diagnostics.
             */
            (void)cert_acceptable(ctx, "explicitly set sender cert", scrt,
                                  NULL, msg);
            if (verify_signature(ctx, msg, scrt))
                return 1;
            CMPerr(0, CMP_R_SRVCERT_DOES_NOT_VALIDATE_MSG);
        }
        break;
    }
    return 0;
}


/*-
 * Check received message (i.e., response by server or request from client)
 * Any msg->extraCerts are prepended to ctx->untrusted_certs
 *
 * Ensures that:
 * it has a valid body type
 * its protection is valid or absent (allowed only if callback function is
 *    present and function yields non-zero result using also supplied argument)
 * its transaction ID matches the previous transaction ID stored in ctx (if any)
 * its recipNonce matches the previous senderNonce stored in the ctx (if any)
 *
 * If everything is fine:
 * learns the senderNonce from the received message,
 * learns the transaction ID if it is not yet in ctx.
 *
 * returns body type (which is >= 0) of the message on success, -1 on error
 */
int ossl_cmp_msg_check_received(OSSL_CMP_CTX *ctx, const OSSL_CMP_MSG *msg,
                                ossl_cmp_allow_unprotected_cb_t cb, int cb_arg)
{
    int rcvd_type;

    if (!ossl_assert(ctx != NULL && msg != NULL))
        return -1;

    if (sk_X509_num(msg->extraCerts) > 10)
        OSSL_CMP_warn(ctx,
                      "received CMP message contains more than 10 extraCerts");
    /*
     * Store any provided extraCerts in ctx for validation and for future use,
     * such that they are also available to ctx->certConf_cb and the peer does
     * not need to send them again in the same transaction.
     * Note that it would not be more secure to add them after checking the msg
     * since they are untrusted and not covered by the CMP protection anyway.
     * For efficiency, the extraCerts are prepended so they get used first.
     */
    if (!ossl_cmp_sk_X509_add1_certs(ctx->untrusted_certs, msg->extraCerts,
                                     0 /* this allows self-signed certs */,
                                     1 /* no_dups */, 1 /* prepend */))
        return -1;

    /* validate message protection */
    if (msg->header->protectionAlg != 0) {
        /* detect explicitly permitted exceptions for invalid protection */
        if (!OSSL_CMP_validate_msg(ctx, msg)
                && (cb == NULL || !(*cb)(ctx, msg, 1, cb_arg))) {
            CMPerr(0, CMP_R_ERROR_VALIDATING_PROTECTION);
            return -1;
        }
    } else {
        /* detect explicitly permitted exceptions for missing protection */
        if (cb == NULL || !(*cb)(ctx, msg, 0, cb_arg)) {
            CMPerr(0, CMP_R_MISSING_PROTECTION);
            return -1;
        }
    }

    /* check CMP version number in header */
    if (ossl_cmp_hdr_get_pvno(OSSL_CMP_MSG_get0_header(msg)) != OSSL_CMP_PVNO) {
        CMPerr(0, CMP_R_UNEXPECTED_PVNO);
        return -1;
    }

    /* compare received transactionID with the expected one in previous msg */
    if (ctx->transactionID != NULL
            && (msg->header->transactionID == NULL
                || ASN1_OCTET_STRING_cmp(ctx->transactionID,
                                         msg->header->transactionID) != 0)) {
        CMPerr(0, CMP_R_TRANSACTIONID_UNMATCHED);
        return -1;
    }

    /* compare received nonce with the one we sent */
    if (ctx->senderNonce != NULL
            && (msg->header->recipNonce == NULL
                    || ASN1_OCTET_STRING_cmp(ctx->senderNonce,
                                             msg->header->recipNonce) != 0)) {
        CMPerr(0, CMP_R_RECIPNONCE_UNMATCHED);
        return -1;
    }

    /*
     * RFC 4210 section 5.1.1 states: the recipNonce is copied from
     * the senderNonce of the previous message in the transaction.
     * --> Store for setting in next message
     */
    if (!ossl_cmp_ctx_set1_recipNonce(ctx, msg->header->senderNonce))
        return -1;

    /* if not yet present, learn transactionID */
    if (ctx->transactionID == NULL
            && !OSSL_CMP_CTX_set1_transactionID(ctx, msg->header->transactionID))
        return -1;

    if ((rcvd_type = ossl_cmp_msg_get_bodytype(msg)) < 0) {
        CMPerr(0, CMP_R_PKIBODY_ERROR);
        return -1;
    }
    return rcvd_type;
}

int ossl_cmp_verify_popo(const OSSL_CMP_MSG *msg, int accept_RAVerified)
{
    if (!ossl_assert(msg != NULL && msg->body != NULL))
        return 0;
    switch (msg->body->type) {
    case OSSL_CMP_PKIBODY_P10CR: {
            X509_REQ *req = msg->body->value.p10cr;
            if (X509_REQ_verify(req, X509_REQ_get0_pubkey(req)) > 0)
                return 1;
            CMPerr(0, CMP_R_REQUEST_NOT_ACCEPTED);
            return 0;
        }
    case OSSL_CMP_PKIBODY_IR:
    case OSSL_CMP_PKIBODY_CR:
    case OSSL_CMP_PKIBODY_KUR:
        return OSSL_CRMF_MSGS_verify_popo(msg->body->value.ir,
                                          OSSL_CMP_CERTREQID,
                                          accept_RAVerified);
    default:
        CMPerr(0, CMP_R_PKIBODY_ERROR);
        return 0;
    }
}
