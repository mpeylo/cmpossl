/*
 * Copyright 2007-2018 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2018
 * Copyright Siemens AG 2015-2018
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * CMP implementation by Martin Peylo, Miikka Viljanen, and David von Oheimb.
 */

/* CMP functions for PKIMessage construction */

#include "cmp_int.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/asn1t.h>
#include <openssl/cmp.h>
#include <openssl/crmf.h>
#include <openssl/err.h>
#include <openssl/x509.h>

OSSL_CMP_PKIHEADER *OSSL_CMP_MSG_get0_header(const OSSL_CMP_MSG *msg)
{
    return msg != NULL ? msg->header : NULL;
}

int OSSL_CMP_MSG_set_bodytype(OSSL_CMP_MSG *msg, int type)
{
    if (msg == NULL || msg->body == NULL)
        return 0;

    msg->body->type = type;

    return 1;
}

int OSSL_CMP_MSG_get_bodytype(const OSSL_CMP_MSG *msg)
{
    if (msg == NULL || msg->body == NULL)
        return -1;

    return msg->body->type;
}

/*
 * Add an extension (or NULL on OOM) to the given extension stack, consuming it.
 *
 * returns 1 on success, 0 on error
 */
static int add_extension(X509_EXTENSIONS **pexts, X509_EXTENSION *ext)
{
    int ret = 0;

    if (pexts == NULL) {
        CMPerr(CMP_F_ADD_EXTENSION, CMP_R_NULL_ARGUMENT);
        goto end;
    }
    if (ext == NULL /* malloc did not work for ext in caller */
            || !X509v3_add_ext(pexts, ext, 0)) {
        CMPerr(CMP_F_ADD_EXTENSION, ERR_R_MALLOC_FAILURE);
        goto end;
    }
    ret = 1;

 end:
    X509_EXTENSION_free(ext);
    return ret;
}

/*
 * Adds a stack of GENERAL_NAMEs to the given extension stack, which may
 * be NULL. This is used to setting subject alternate names to a certTemplate.
 *
 * returns 1 on success, 0 on error
 */
#define ADD_SANs(pexts, sans, critical) \
    add_extension(pexts, X509V3_EXT_i2d(NID_subject_alt_name, critical, sans))

/*
 * Adds a CERTIFICATEPOLICIES structure to the given extension stack, which may
 * be NULL. This is used to setting certificate policy OIDs to a certTemplate.
 *
 * returns 1 on success, 0 on error
 */
#define ADD_POLICIES(pexts, policies, critical) add_extension(pexts, \
               X509V3_EXT_i2d(NID_certificate_policies, critical, policies))

/*
 * Adds a CRL revocation reason code to an extension stack (which may be NULL)
 * returns 1 on success, 0 on error
 */
static int add_crl_reason_extension(X509_EXTENSIONS **pexts, int reason_code)
{
    ASN1_ENUMERATED *val = NULL;
    X509_EXTENSION *ext = NULL;
    int ret = 0;

    if (((val = ASN1_ENUMERATED_new()) != NULL)
            && ASN1_ENUMERATED_set(val, reason_code))
        ext = X509V3_EXT_i2d(NID_crl_reason, 0, val);
    ret = add_extension(pexts, ext);
    ASN1_ENUMERATED_free(val);
    return ret;
}

OSSL_CMP_MSG *OSSL_CMP_MSG_create(OSSL_CMP_CTX *ctx, int bodytype)
{
    OSSL_CMP_MSG *msg = NULL;

    if (ctx == NULL) {
        CMPerr(CMP_F_OSSL_CMP_MSG_CREATE, CMP_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_new()) == NULL)
        goto oom;
    if (!OSSL_CMP_HDR_init(ctx, msg->header)
            || !OSSL_CMP_MSG_set_bodytype(msg, bodytype)
            || (ctx->geninfo_itavs != NULL
                && !OSSL_CMP_HDR_generalInfo_push1_items(msg->header,
                                                         ctx->geninfo_itavs)))
        goto err;

    switch (bodytype) {
    case OSSL_CMP_PKIBODY_IR:
    case OSSL_CMP_PKIBODY_CR:
    case OSSL_CMP_PKIBODY_KUR:
        if ((msg->body->value.ir = OSSL_CRMF_MSGS_new()) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_P10CR:
        if (ctx->p10CSR == NULL) {
            CMPerr(CMP_F_OSSL_CMP_MSG_CREATE, CMP_R_ERROR_CREATING_P10CR);
            goto err;
        }
        if ((msg->body->value.p10cr = X509_REQ_dup(ctx->p10CSR)) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_IP:
    case OSSL_CMP_PKIBODY_CP:
    case OSSL_CMP_PKIBODY_KUP:
        if ((msg->body->value.ip = OSSL_CMP_CERTREPMESSAGE_new()) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_RR:
        if ((msg->body->value.rr = sk_OSSL_CMP_REVDETAILS_new_null()) == NULL)
            goto oom;
        return msg;
    case OSSL_CMP_PKIBODY_RP:
        if ((msg->body->value.rp = OSSL_CMP_REVREPCONTENT_new()) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_CERTCONF:
        if ((msg->body->value.certConf =
             sk_OSSL_CMP_CERTSTATUS_new_null()) == NULL)
            goto oom;
        return msg;
    case OSSL_CMP_PKIBODY_PKICONF:
        if ((msg->body->value.pkiconf = ASN1_TYPE_new()) == NULL)
            goto oom;
        ASN1_TYPE_set(msg->body->value.pkiconf, V_ASN1_NULL, NULL);
        return msg;

    case OSSL_CMP_PKIBODY_POLLREQ:
        if ((msg->body->value.pollReq = sk_OSSL_CMP_POLLREQ_new_null()) == NULL)
            goto oom;
        return msg;
    case OSSL_CMP_PKIBODY_POLLREP:
        if ((msg->body->value.pollRep = sk_OSSL_CMP_POLLREP_new_null()) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_GENM:
    case OSSL_CMP_PKIBODY_GENP:
        if ((msg->body->value.genm = sk_OSSL_CMP_ITAV_new_null()) == NULL)
            goto oom;
        return msg;

    case OSSL_CMP_PKIBODY_ERROR:
        if ((msg->body->value.error = OSSL_CMP_ERRORMSGCONTENT_new()) == NULL)
            goto oom;
        return msg;

    default:
        CMPerr(CMP_F_OSSL_CMP_MSG_CREATE, CMP_R_UNEXPECTED_PKIBODY);
        goto err;
    }
oom:
    CMPerr(CMP_F_OSSL_CMP_MSG_CREATE, ERR_R_MALLOC_FAILURE);
err:
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

#define HAS_SAN(ctx) (sk_GENERAL_NAME_num((ctx)->subjectAltNames) > 0 \
                          || OSSL_CMP_CTX_reqExtensions_have_SAN(ctx))
static X509_NAME *determine_subj(OSSL_CMP_CTX *ctx, X509 *refcert,
                                 int bodytype) {
    if (ctx->subjectName != NULL) {
        return ctx->subjectName;
    }
    if (refcert != NULL
            && (bodytype == OSSL_CMP_PKIBODY_KUR || !HAS_SAN(ctx)))
        /*
         * For KUR, copy subjectName from reference certificate.
         * For IR or CR, do the same only if there is no subjectAltName.
         */
        return X509_get_subject_name(refcert);
    return NULL;
}

/*
 * Create CRMF certificate request message for IR/CR/KUR
 * returns a pointer to the OSSL_CRMF_MSG on success, NULL on error
 */
static OSSL_CRMF_MSG *crm_new(OSSL_CMP_CTX *ctx, int bodytype,
                              int rid, EVP_PKEY *rkey)
{
    OSSL_CRMF_MSG *crm = NULL;
    X509 *refcert = ctx->oldClCert != NULL ? ctx->oldClCert : ctx->clCert;
       /* refcert defaults to current client cert */
    STACK_OF(GENERAL_NAME) *default_sans = NULL;
    X509_NAME *subject = determine_subj(ctx, refcert, bodytype);
    int crit = ctx->setSubjectAltNameCritical || subject == NULL;
    /* RFC5280: subjectAltName MUST be critical if subject is null */
    X509_EXTENSIONS *exts = NULL;

    if (rkey == NULL
            || (bodytype == OSSL_CMP_PKIBODY_KUR && refcert == NULL)) {
        CMPerr(CMP_F_CRM_NEW, CMP_R_INVALID_ARGS);
        goto err;
    }
    if ((crm = OSSL_CRMF_MSG_new()) == NULL)
        goto oom;
    if (!OSSL_CRMF_MSG_set_certReqId(crm, rid)

    /* fill certTemplate, corresponding to PKCS#10 CertificationRequestInfo */
            /* rkey cannot be NULL so far - but it can be when
             * centralized key creation is supported --> GitHub issue#68 */
            || !OSSL_CRMF_CERTTEMPLATE_fill(OSSL_CRMF_MSG_get0_tmpl(crm), rkey,
                                            subject, ctx->issuer,
                                            NULL/* serial */))
        goto err;
    if (ctx->days != 0) {
        time_t notBefore, notAfter;
        notBefore = time(NULL);
        notAfter = notBefore + 60 * 60 * 24 * ctx->days;
        if (!OSSL_CRMF_MSG_set_validity(crm, notBefore, notAfter))
            goto err;
    }

    /* extensions */
    if (refcert != NULL && !ctx->SubjectAltName_nodefault)
        default_sans = X509V3_get_d2i(X509_get0_extensions(refcert),
                                      NID_subject_alt_name, NULL, NULL);
    /* exts are copied from ctx to allow reuse */
    exts = CMP_X509_EXTENSIONS_dup(ctx->reqExtensions);
    if (ctx->reqExtensions != NULL && exts == NULL)
        goto oom;
    if (sk_GENERAL_NAME_num(ctx->subjectAltNames) > 0
            && !ADD_SANs(&exts, ctx->subjectAltNames, crit))
        goto err;
    if (!HAS_SAN(ctx) && default_sans != NULL
            && !ADD_SANs(&exts, default_sans, crit))
        goto err;
    if (ctx->policies != NULL
            && !ADD_POLICIES(&exts, ctx->policies, ctx->setPoliciesCritical))
        goto err;
    if (!OSSL_CRMF_MSG_set0_extensions(crm, exts))
        goto err;
    sk_GENERAL_NAME_pop_free(default_sans, GENERAL_NAME_free);
    exts = NULL;
    /* end fill certTemplate, now set any controls */

    /* for KUR, set OldCertId according to D.6 */
    if (bodytype == OSSL_CMP_PKIBODY_KUR) {
        OSSL_CRMF_CERTID *cid =
            OSSL_CRMF_CERTID_gen(X509_get_issuer_name(refcert),
                                 X509_get_serialNumber(refcert));
        int ret;

        if (cid == NULL)
            goto err;
        ret = OSSL_CRMF_MSG_set1_regCtrl_oldCertID(crm, cid);
        OSSL_CRMF_CERTID_free(cid);
        if (ret == 0)
            goto err;
    }

    return crm;

 oom:
    CMPerr(CMP_F_CRM_NEW, ERR_R_MALLOC_FAILURE);
 err:
    sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
    sk_GENERAL_NAME_pop_free(default_sans, GENERAL_NAME_free);
    OSSL_CRMF_MSG_free(crm);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_certreq_new(OSSL_CMP_CTX *ctx, int type,int err_code)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CRMF_MSG *crm = NULL;

    if (ctx == NULL
          || (type != OSSL_CMP_PKIBODY_P10CR && ctx->pkey == NULL
                  && ctx->newPkey == NULL)
          || (type != OSSL_CMP_PKIBODY_IR && type != OSSL_CMP_PKIBODY_CR
                  && type != OSSL_CMP_PKIBODY_KUR
                  && type != OSSL_CMP_PKIBODY_P10CR)) {
        CMPerr(CMP_F_OSSL_CMP_CERTREQ_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, type)) == NULL)
        goto err;

    /* header */
    if (ctx->implicitConfirm)
        if (!OSSL_CMP_HDR_set_implicitConfirm(msg->header))
            goto err;

    /* body */
    /* For P10CR the content has already been set in OSSL_CMP_MSG_create */
     if (type != OSSL_CMP_PKIBODY_P10CR) {
        EVP_PKEY *rkey = ctx->newPkey != NULL ? ctx->newPkey
            : ctx->pkey; /* default is currenty client key */

        if ((crm = crm_new(ctx, type, OSSL_CMP_CERTREQID, rkey)) == NULL
                || !OSSL_CRMF_MSG_create_popo(crm, rkey, ctx->digest,
                                              ctx->popoMethod)
                /* value.ir is same for cr and kur */
                || !sk_OSSL_CRMF_MSG_push(msg->body->value.ir, crm))
            goto err;
        crm = NULL;
        /* TODO: here optional 2nd certreqmsg could be pushed to the stack */
    }

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_CERTREQ_NEW, err_code);
    OSSL_CRMF_MSG_free(crm);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_certrep_new(OSSL_CMP_CTX *ctx, int bodytype,
                                   int certReqId, OSSL_CMP_PKISI *si,
                                   X509 *cert, STACK_OF(X509) *chain,
                                   STACK_OF(X509) *caPubs, int encrypted,
                                   int unprotectedErrors)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CMP_CERTREPMESSAGE *repMsg = NULL;
    OSSL_CMP_CERTRESPONSE *resp = NULL;
    int status = -1;

    if (ctx == NULL || si == NULL) {
        CMPerr(CMP_F_OSSL_CMP_CERTREP_NEW, CMP_R_NULL_ARGUMENT);
        goto err;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, bodytype)) == NULL)
        goto oom;
    repMsg = msg->body->value.ip; /* value.ip is same for cp and kup */

    /* header */
    if (ctx->implicitConfirm && !OSSL_CMP_HDR_set_implicitConfirm(msg->header))
        goto oom;

    /* body */
    if ((resp = OSSL_CMP_CERTRESPONSE_new()) == NULL)
        goto oom;
    OSSL_CMP_PKISI_free(resp->status);
    if ((resp->status = OSSL_CMP_PKISI_dup(si)) == NULL
            || !ASN1_INTEGER_set(resp->certReqId, certReqId)) {
        goto oom;
    }

    status = OSSL_CMP_PKISI_get_PKIStatus(resp->status);
    if (status != OSSL_CMP_PKISTATUS_rejection
            && status != OSSL_CMP_PKISTATUS_waiting && cert != NULL) {
        if (encrypted) {
            CMPerr(CMP_F_OSSL_CMP_CERTREP_NEW, CMP_R_INVALID_PARAMETERS);
            goto err;
        } else {
            if ((resp->certifiedKeyPair = OSSL_CMP_CERTIFIEDKEYPAIR_new())
                == NULL)
                goto oom;
            resp->certifiedKeyPair->certOrEncCert->type =
                OSSL_CMP_CERTORENCCERT_CERTIFICATE;
            if (!X509_up_ref(cert))
                goto err;
            resp->certifiedKeyPair->certOrEncCert->value.certificate = cert;
        }
    }

    if (!sk_OSSL_CMP_CERTRESPONSE_push(repMsg->response, resp))
        goto oom;
    resp = NULL;
    /* TODO: here optional 2nd certrep could be pushed to the stack */

    if (bodytype == OSSL_CMP_PKIBODY_IP && caPubs != NULL
            && (repMsg->caPubs = X509_chain_up_ref(caPubs)) == NULL)
        goto oom;
    if (chain != NULL
            && !OSSL_CMP_sk_X509_add1_certs(msg->extraCerts, chain, 0, 1))
        goto oom;

    if (!(unprotectedErrors
            && OSSL_CMP_PKISI_get_PKIStatus(si) == OSSL_CMP_PKISTATUS_rejection)
            && !OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 oom:
    CMPerr(CMP_F_OSSL_CMP_CERTREP_NEW, ERR_R_MALLOC_FAILURE);
 err:
    CMPerr(CMP_F_OSSL_CMP_CERTREP_NEW, CMP_R_ERROR_CREATING_CERTREP);
    OSSL_CMP_CERTRESPONSE_free(resp);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_rr_new(OSSL_CMP_CTX *ctx)
{
    OSSL_CMP_MSG *msg = NULL;
    EVP_PKEY *pubkey = NULL;
    OSSL_CMP_REVDETAILS *rd = NULL;
    int ret;

    if (ctx == NULL || ctx->oldClCert == NULL) {
        CMPerr(CMP_F_OSSL_CMP_RR_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_RR)) == NULL)
        goto err;

    if ((rd = OSSL_CMP_REVDETAILS_new()) == NULL)
        goto err;
    sk_OSSL_CMP_REVDETAILS_push(msg->body->value.rr, rd);

    /* Fill the template from the contents of the certificate to be revoked */
    if ((pubkey = X509_get_pubkey(ctx->oldClCert)) == NULL)
        goto err;
    ret = OSSL_CRMF_CERTTEMPLATE_fill(rd->certDetails,
                                      NULL/* pubkey would be redundant */,
                                      NULL/* subject would be redundant */,
                                      X509_get_issuer_name(ctx->oldClCert),
                                      X509_get_serialNumber(ctx->oldClCert));
    EVP_PKEY_free(pubkey);
    if (ret == 0)
        goto err;

    /* revocation reason code is optional */
    if (ctx->revocationReason != CRL_REASON_NONE
            && !add_crl_reason_extension(&rd->crlEntryDetails,
                                         ctx->revocationReason))
        goto err;

    /*
     * TODO: the Revocation Passphrase according to section 5.3.19.9 could be
     *       set here if set in ctx
     */

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_RR_NEW, CMP_R_ERROR_CREATING_RR);
    OSSL_CMP_MSG_free(msg);

    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_rp_new(OSSL_CMP_CTX *ctx, OSSL_CMP_PKISI *si,
                              OSSL_CRMF_CERTID *cid, int unprot_err)
{
    OSSL_CMP_REVREPCONTENT *rep = NULL;
    OSSL_CMP_PKISI *si1 = NULL;
    OSSL_CMP_MSG *msg = NULL;

    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_RP)) == NULL)
        goto oom;
    rep = msg->body->value.rp;

    if ((si1 = OSSL_CMP_PKISI_dup(si)) == NULL)
        goto oom;
    sk_OSSL_CMP_PKISI_push(rep->status, si1);

    if ((rep->revCerts = sk_OSSL_CRMF_CERTID_new_null()) == NULL)
        goto oom;
    sk_OSSL_CRMF_CERTID_push(rep->revCerts, cid);
    cid = NULL;

    if (!(unprot_err
          && OSSL_CMP_PKISI_get_PKIStatus(si) == OSSL_CMP_PKISTATUS_rejection)
          && !OSSL_CMP_MSG_protect(ctx, msg))
        goto err;
    return msg;

 oom:
    CMPerr(CMP_F_OSSL_CMP_RP_NEW, ERR_R_MALLOC_FAILURE);
 err:
    CMPerr(CMP_F_OSSL_CMP_RP_NEW, CMP_R_ERROR_CREATING_RP);
    OSSL_CRMF_CERTID_free(cid);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_pkiconf_new(OSSL_CMP_CTX *ctx)
{
    OSSL_CMP_MSG *msg =
        OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_PKICONF);

    if (msg == NULL)
        goto err;
    if (OSSL_CMP_MSG_protect(ctx, msg))
        return msg;
 err:
    CMPerr(CMP_F_OSSL_CMP_PKICONF_NEW, CMP_R_ERROR_CREATING_PKICONF);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

int OSSL_CMP_MSG_gen_push0_ITAV(OSSL_CMP_MSG *msg, OSSL_CMP_ITAV *itav)
{
    int bodytype;

    if (msg == NULL)
        goto err;
    bodytype = OSSL_CMP_MSG_get_bodytype(msg);
    if (bodytype != OSSL_CMP_PKIBODY_GENM && bodytype != OSSL_CMP_PKIBODY_GENP)
        goto err;

    /* value.genp has the same structure, so this works for genp as well */
    if (!OSSL_CMP_ITAV_push0_stack_item(&msg->body->value.genm, itav))
        goto err;
    return 1;

 err:
    CMPerr(CMP_F_OSSL_CMP_MSG_GEN_PUSH0_ITAV, CMP_R_INVALID_ARGS);
    return 0;
}

int OSSL_CMP_MSG_gen_push1_ITAVs(OSSL_CMP_MSG *msg,
                                 STACK_OF(OSSL_CMP_ITAV) *itavs)
{
    int i;
    OSSL_CMP_ITAV *itav = NULL;

    if (msg == NULL)
        goto err;

    for (i = 0; i < sk_OSSL_CMP_ITAV_num(itavs); i++) {
        itav = OSSL_CMP_ITAV_dup(sk_OSSL_CMP_ITAV_value(itavs,i));
        if (!OSSL_CMP_MSG_gen_push0_ITAV(msg, itav)) {
            OSSL_CMP_ITAV_free(itav);
            goto err;
        }
    }
    return 1;

 err:
    CMPerr(CMP_F_OSSL_CMP_MSG_GEN_PUSH1_ITAVS, CMP_R_INVALID_ARGS);
    return 0;
}

/*
 * Creates a new General Message/Response with an empty itav stack
 * returns a pointer to the PKIMessage on success, NULL on error
 */
static OSSL_CMP_MSG *CMP_gen_new(OSSL_CMP_CTX *ctx, int body_type,
                                        int err_code)
{
    OSSL_CMP_MSG *msg = NULL;

    if (ctx == NULL) {
        CMPerr(CMP_F_CMP_GEN_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, body_type)) == NULL)
        goto err;

    if (ctx->genm_itavs)
        if (!OSSL_CMP_MSG_gen_push1_ITAVs(msg, ctx->genm_itavs))
            goto err;

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_CMP_GEN_NEW, err_code);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_genm_new(OSSL_CMP_CTX *ctx)
{
    return CMP_gen_new(ctx, OSSL_CMP_PKIBODY_GENM, CMP_R_ERROR_CREATING_GENM);
}

OSSL_CMP_MSG *OSSL_CMP_genp_new(OSSL_CMP_CTX *ctx)
{
    return CMP_gen_new(ctx, OSSL_CMP_PKIBODY_GENP, CMP_R_ERROR_CREATING_GENP);
}

OSSL_CMP_MSG *OSSL_CMP_error_new(OSSL_CMP_CTX *ctx, OSSL_CMP_PKISI *si,
                                 int errorCode,
                                 OSSL_CMP_PKIFREETEXT *errorDetails,
                                 int unprotected)
{
    OSSL_CMP_MSG *msg = NULL;

    if (ctx == NULL || si == NULL) {
        CMPerr(CMP_F_OSSL_CMP_ERROR_NEW, CMP_R_NULL_ARGUMENT);
        goto err;
    }
    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_ERROR)) == NULL)
        goto err;

    OSSL_CMP_PKISI_free(msg->body->value.error->pKIStatusInfo);
    msg->body->value.error->pKIStatusInfo = OSSL_CMP_PKISI_dup(si);
    if (errorCode >= 0) {
        msg->body->value.error->errorCode = ASN1_INTEGER_new();
        if (!ASN1_INTEGER_set(msg->body->value.error->errorCode, errorCode))
            goto err;
    }
    if (errorDetails != NULL)
        msg->body->value.error->errorDetails = sk_ASN1_UTF8STRING_deep_copy(
                            errorDetails, ASN1_STRING_dup, ASN1_STRING_free);

    if (!unprotected && !OSSL_CMP_MSG_protect(ctx, msg))
        goto err;
    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_ERROR_NEW, CMP_R_ERROR_CREATING_ERROR);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

/*
 * OSSL_CMP_CERTSTATUS_set_certHash() calculates a hash of the certificate,
 * using the same hash algorithm as is used to create and verify the
 * certificate signature, and places the hash into the certHash field of a
 * OSSL_CMP_CERTSTATUS structure. This is used in the certConf message,
 * for example, to confirm that the certificate was received successfully.
 */
int OSSL_CMP_CERTSTATUS_set_certHash(OSSL_CMP_CERTSTATUS *certStatus,
                                const X509 *cert)
{
    unsigned int len;
    unsigned char hash[EVP_MAX_MD_SIZE];
    int md_NID;
    const EVP_MD *md = NULL;

    if (certStatus == NULL || cert == NULL)
        goto err;

    /*-
     * select hash algorithm, as stated in Appendix F. Compilable ASN.1 defs:
     * the hash of the certificate, using the same hash algorithm
     * as is used to create and verify the certificate signature
     */
    if (OBJ_find_sigid_algs(X509_get_signature_nid(cert), &md_NID, NULL)
            && (md = EVP_get_digestbynid(md_NID)) != NULL) {
        if (!X509_digest(cert, md, hash, &len))
            goto err;
        if (!CMP_ASN1_OCTET_STRING_set1_bytes(&certStatus->certHash, hash, len))
            goto err;
    } else {
        CMPerr(CMP_F_OSSL_CMP_CERTSTATUS_SET_CERTHASH,
               CMP_R_UNSUPPORTED_ALGORITHM);
        goto err;
    }

    return 1;
 err:
    CMPerr(CMP_F_OSSL_CMP_CERTSTATUS_SET_CERTHASH, CMP_R_ERROR_SETTING_CERTHASH);
    return 0;
}

/*
 * TODO: handle potential 2nd certificate when signing and encrypting
 * certificates have been requested/received
 */
OSSL_CMP_MSG *OSSL_CMP_certConf_new(OSSL_CMP_CTX *ctx, int fail_info,
                                    const char *text)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CMP_CERTSTATUS *certStatus = NULL;
    OSSL_CMP_PKISI *sinfo;

    if (ctx == NULL || ctx->newClCert == NULL) {
        CMPerr(CMP_F_OSSL_CMP_CERTCONF_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }
    if ((unsigned)fail_info > OSSL_CMP_PKIFAILUREINFO_MAX_BIT_PATTERN)
        CMPerr(CMP_F_OSSL_CMP_CERTCONF_NEW, CMP_R_FAIL_INFO_OUT_OF_RANGE);

    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_CERTCONF)) == NULL)
        goto err;

    if ((certStatus = OSSL_CMP_CERTSTATUS_new()) == NULL)
        goto err;
    if (!sk_OSSL_CMP_CERTSTATUS_push(msg->body->value.certConf, certStatus))
        goto err;
    /* set the ID of the certReq */
    ASN1_INTEGER_set(certStatus->certReqId, OSSL_CMP_CERTREQID);
    /*
     * the hash of the certificate, using the same hash algorithm
     * as is used to create and verify the certificate signature
     */
    if (!OSSL_CMP_CERTSTATUS_set_certHash(certStatus, ctx->newClCert))
        goto err;
    /*
     * For any particular CertStatus, omission of the statusInfo field
     * indicates ACCEPTANCE of the specified certificate.  Alternatively,
     * explicit status details (with respect to acceptance or rejection) MAY
     * be provided in the statusInfo field, perhaps for auditing purposes at
     * the CA/RA.
     */
    sinfo = fail_info != 0 ?
        OSSL_CMP_statusInfo_new(OSSL_CMP_PKISTATUS_rejection, fail_info, text) :
        OSSL_CMP_statusInfo_new(OSSL_CMP_PKISTATUS_accepted, 0, text);
    if (sinfo == NULL)
        goto err;
    certStatus->statusInfo = sinfo;

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_CERTCONF_NEW, CMP_R_ERROR_CREATING_CERTCONF);
    OSSL_CMP_MSG_free(msg);

    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_pollReq_new(OSSL_CMP_CTX *ctx, int crid)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CMP_POLLREQ *preq = NULL;

    if (ctx == NULL
          || (msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_POLLREQ)) == NULL)
        goto err;

    /* TODO: support multiple cert request IDs to poll */
    if ((preq = OSSL_CMP_POLLREQ_new()) == NULL
            || !ASN1_INTEGER_set(preq->certReqId, crid)
            || !sk_OSSL_CMP_POLLREQ_push(msg->body->value.pollReq, preq))
        goto err;

    preq = NULL;
    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_POLLREQ_NEW, CMP_R_ERROR_CREATING_POLLREQ);
    OSSL_CMP_POLLREQ_free(preq);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_pollRep_new(OSSL_CMP_CTX *ctx, int crid,
                                   int64_t poll_after)
{
    OSSL_CMP_MSG *msg;
    OSSL_CMP_POLLREP *prep;

    if (ctx == NULL) {
        CMPerr(CMP_F_OSSL_CMP_POLLREP_NEW, CMP_R_NULL_ARGUMENT);
        return NULL;
    }
    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_POLLREP)) == NULL)
        goto err;
    if ((prep = OSSL_CMP_POLLREP_new()) == NULL)
        goto err;
    sk_OSSL_CMP_POLLREP_push(msg->body->value.pollRep, prep);
    ASN1_INTEGER_set(prep->certReqId, crid);
    ASN1_INTEGER_set_int64(prep->checkAfter, poll_after);

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;
    return msg;

 err:
    CMPerr(CMP_F_OSSL_CMP_POLLREP_NEW, CMP_R_ERROR_CREATING_POLLREP);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

OSSL_CMP_MSG *OSSL_CMP_MSG_load(const char *file)
{
    OSSL_CMP_MSG *msg = NULL;
    BIO *bio = NULL;

    if (file == NULL || (bio = BIO_new_file(file, "rb")) == NULL)
        return NULL;
    msg = OSSL_d2i_CMP_MSG_bio(bio, NULL);
    BIO_free(bio);
    return msg;
}
