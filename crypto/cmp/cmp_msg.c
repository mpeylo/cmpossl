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

#include "cmp_local.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/asn1t.h>
#include <openssl/cmp.h>
#include <openssl/crmf.h>
#include <openssl/err.h>
#include <openssl/x509.h>

const char *ossl_cmp_bodytype_to_string(int type)
{
    static const char *type_names[] = {
        "IR", "IP", "CR", "CP", "P10CR",
        "POPDECC", "POPDECR", "KUR", "KUP",
        "KRR", "KRP", "RR", "RP", "CCR", "CCP",
        "CKUANN", "CANN", "RANN", "CRLANN", "PKICONF", "NESTED",
        "GENM", "GENP", "ERROR", "CERTCONF", "POLLREQ", "POLLREP",
    };

    if (type < 0 || type > OSSL_CMP_PKIBODY_TYPE_MAX)
        return "illegal body type";
    return type_names[type];
}

/*
 * Add an extension (or NULL on OOM) to the given extension stack, consuming it.
 *
 * returns 1 on success, 0 on error
 */
static int add_extension(X509_EXTENSIONS **exts, X509_EXTENSION *ext)
{
    int ret = 0;

    if (exts == NULL) {
        CMPerr(CMP_F_ADD_EXTENSION, CMP_R_NULL_ARGUMENT);
        goto end;
    }
    if (ext == NULL || /* malloc did not work for ext in caller */
        !X509v3_add_ext(exts, ext, 0)) {
        CMPerr(CMP_F_ADD_EXTENSION, ERR_R_MALLOC_FAILURE);
        goto end;
    }
    ret = 1;

 end:
    X509_EXTENSION_free(ext);
    return ret;
}

/*
 * Takes a stack of GENERAL_NAMEs and adds them to the given extension stack.
 * this is used to setting subject alternate names to a certTemplate
 *
 * returns 1 on success, 0 on error
 */
#define ADD_SANs(exts, sans, critical) \
    add_extension(exts, X509V3_EXT_i2d(NID_subject_alt_name, critical, sans))

/*
 * Takes a CERTIFICATEPOLICIES structure and adds it to the given extension
 * stack. This is used to setting certificate policy OIDs to a certTemplate
 *
 * returns 1 on success, 0 on error
 */
#define ADD_POLICIES(exts, policies, critical) add_extension(exts, \
               X509V3_EXT_i2d(NID_certificate_policies, critical, policies))

/* Add extension list to the referenced extension stack, which may be NULL */
static int add_extensions(STACK_OF(X509_EXTENSION) **target,
                          const STACK_OF(X509_EXTENSION) *exts)
{
    int i;

    if (target == NULL)
        return 0;

    for (i = 0; i < sk_X509_EXTENSION_num(exts); i++) {
        X509_EXTENSION *ext = sk_X509_EXTENSION_value(exts, i);
        ASN1_OBJECT *obj = X509_EXTENSION_get_object(ext);
        int idx = X509v3_get_ext_by_OBJ(*target, obj, -1);

        /* Does extension exist in target? */
        if (idx != -1) {
            /* Delete all extensions of same type */
            do {
                X509_EXTENSION_free(sk_X509_EXTENSION_delete(*target, idx));
                idx = X509v3_get_ext_by_OBJ(*target, obj, -1);
            } while (idx != -1);
        }
        if (!X509v3_add_ext(target, ext, -1))
            return 0;
    }
    return 1;
}

/*
 * Adds a CRL revocation reason code to an extension stack (which may be NULL)
 * returns 1 on success, 0 on error
 */
static int add_crl_reason_extension(X509_EXTENSIONS **exts, int reason_code)
{
    ASN1_ENUMERATED *val = NULL;
    X509_EXTENSION *ext = NULL;
    int ret = 0;

    if (((val = ASN1_ENUMERATED_new()) != NULL) &&
        ASN1_ENUMERATED_set(val, reason_code))
        ext = X509V3_EXT_i2d(NID_crl_reason, 0, val);
    ret = add_extension(exts, ext);
    ASN1_ENUMERATED_free(val);
    return ret;
}


/*
 * Creates and initializes a OSSL_CMP_MSG structure, using ctx for
 * the header and bodytype for the body.
 * returns pointer to created OSSL_CMP_MSG on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_MSG_create(OSSL_CMP_CTX *ctx, int bodytype)
{
    OSSL_CMP_MSG *msg = NULL;

    if (ctx == NULL) {
        CMPerr(CMP_F_OSSL_CMP_MSG_CREATE, CMP_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_new()) == NULL)
        goto oom;
    if (!OSSL_CMP_PKIHEADER_init(ctx, msg->header) ||
        !OSSL_CMP_MSG_set_bodytype(msg, bodytype) ||
        (ctx->geninfo_itavs != NULL &&
         !OSSL_CMP_MSG_generalInfo_items_push1(msg, ctx->geninfo_itavs)))
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

X509_EXTENSIONS *CMP_exts_dup(const X509_EXTENSIONS *extin /* may be NULL */)
{
    X509_EXTENSIONS *exts = sk_X509_EXTENSION_new_null();

    if (exts == NULL)
        goto err;
    if (extin != NULL) {
        int i;
        for (i = 0; i < sk_X509_EXTENSION_num(extin); i++)
            if (!sk_X509_EXTENSION_push(exts, X509_EXTENSION_dup(
                                        sk_X509_EXTENSION_value(extin, i))))
                goto err;
    }
    return exts;

 err:
    sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
    return NULL;
}

#define HAS_SAN(ctx) (sk_GENERAL_NAME_num((ctx)->subjectAltNames) > 0 || \
                      OSSL_CMP_CTX_reqExtensions_have_SAN(ctx))
static X509_NAME *determine_subj(OSSL_CMP_CTX *ctx,
                                 X509_NAME *ref_subj,
                                 int for_KUR) {
    if (ctx->subjectName != NULL) {
        return ctx->subjectName;
    }
    if (ref_subj != NULL && (for_KUR || !HAS_SAN(ctx)))
        /*
         * For KUR, copy subjectName from the reference.
         * For IR or CR, do the same only if there is no subjectAltName.
         */
        return ref_subj;
    return NULL;
}

/*
 * Create CRMF certificate request message for IR/CR/KUR
 * returns a pointer to the OSSL_CRMF_MSG on success, NULL on error
 */
static OSSL_CRMF_MSG *crm_new(OSSL_CMP_CTX *ctx, int bodytype, int rid)
{
    int for_KUR = bodytype == OSSL_CMP_PKIBODY_KUR;
    OSSL_CRMF_MSG *crm = NULL;
    X509 *refcert = ctx->oldCert != NULL ? ctx->oldCert : ctx->clCert;
    /* refcert defaults to current client cert */
    EVP_PKEY *rkey = OSSL_CMP_CTX_get0_newPkey(ctx, /* ignored */ 0);
    STACK_OF(GENERAL_NAME) *default_sans = NULL;
    X509_NAME *ref_subj =
        ctx->p10CSR != NULL ? X509_REQ_get_subject_name(ctx->p10CSR) :
        refcert != NULL ? X509_get_subject_name(refcert) : NULL;
    X509_NAME *subject = determine_subj(ctx, ref_subj, for_KUR);
    X509_NAME *issuer = ctx->issuer != NULL || refcert == NULL
        ? ctx->issuer : X509_get_issuer_name(refcert);
    int crit = ctx->setSubjectAltNameCritical || subject == NULL;
    /* RFC5280: subjectAltName MUST be critical if subject is null */
    X509_EXTENSIONS *exts = NULL;

    if (rkey == NULL && ctx->p10CSR != NULL)
        rkey = X509_REQ_get0_pubkey(ctx->p10CSR);
    if (rkey == NULL)
        rkey = ctx->pkey; /* default is independent of ctx->oldCert */
    if (rkey == NULL) {
        CMPerr(CMP_F_CRM_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }
    if (for_KUR && refcert == NULL && ctx->p10CSR == NULL) {
        CMPerr(CMP_F_CRM_NEW, CMP_R_MISSING_REFERENCE_CERT);
        return NULL;
    }
    if ((crm = OSSL_CRMF_MSG_new()) == NULL)
        goto oom;
    if (!OSSL_CRMF_MSG_set_certReqId(crm, rid) ||

    /* fill certTemplate, corresponding to PKCS#10 CertificationRequestInfo */
            /* rkey is not allowed to be NULL so far - but it can be when
             * centralized key creation is supported --> GitHub issue#68 */
        !OSSL_CRMF_CERTTEMPLATE_fill(OSSL_CRMF_MSG_get0_tmpl(crm), rkey,
                                     subject, issuer, NULL/* serial */))
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
    if (ctx->p10CSR != NULL
            && (exts = X509_REQ_get_extensions(ctx->p10CSR)) == NULL)
        goto err;
    if (ctx->reqExtensions != NULL /* augment/override existing ones */
            && !add_extensions(&exts, ctx->reqExtensions))
        goto err;
    if ((sk_GENERAL_NAME_num(ctx->subjectAltNames) > 0 &&
         !ADD_SANs(&exts, ctx->subjectAltNames, crit)) ||
        (!HAS_SAN(ctx) && default_sans != NULL &&
         !ADD_SANs(&exts, default_sans, crit)) ||
        (ctx->policies != NULL &&
         !ADD_POLICIES(&exts, ctx->policies, ctx->setPoliciesCritical)) ||
        !OSSL_CRMF_MSG_set0_extensions(crm, exts))
        goto err;
    sk_GENERAL_NAME_pop_free(default_sans, GENERAL_NAME_free);
    exts = NULL;
    /* end fill certTemplate, now set any controls */

    /* for KUR, set OldCertId according to D.6 */
    if (for_KUR && refcert != NULL) {
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

/*
 * Create certificate request PKIMessage for IR/CR/KUR/P10CR
 * returns a pointer to the PKIMessage on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_certreq_new(OSSL_CMP_CTX *ctx, int type, int err_code)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CRMF_MSG *crm = NULL;

    if (ctx == NULL ||
        (type != OSSL_CMP_PKIBODY_P10CR && ctx->pkey == NULL &&
         ctx->newPkey == NULL) ||
         (type != OSSL_CMP_PKIBODY_IR && type != OSSL_CMP_PKIBODY_CR &&
          type != OSSL_CMP_PKIBODY_KUR && type != OSSL_CMP_PKIBODY_P10CR)) {
        CMPerr(CMP_F_OSSL_CMP_CERTREQ_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, type)) == NULL)
        goto err;

    /* header */
    if (ctx->implicit_confirm)
        if (!OSSL_CMP_MSG_set_implicitConfirm(msg))
            goto err;

    /* body */
    /* For P10CR the content has already been set in OSSL_CMP_MSG_create */
    if (type != OSSL_CMP_PKIBODY_P10CR) {
        EVP_PKEY *privkey = OSSL_CMP_CTX_get0_newPkey(ctx, /* ignored */ 1);

        if (privkey == NULL)
            privkey = ctx->pkey; /* default is independent of ctx->oldCert */
        if ((crm = crm_new(ctx, type, OSSL_CMP_CERTREQID)) == NULL
            || !OSSL_CRMF_MSG_create_popo(crm, privkey, ctx->digest,
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

/*
 * Create certificate response PKIMessage for IP/CP/KUP
 * returns a pointer to the PKIMessage on success, NULL on error
 */
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
    if (ctx->implicit_confirm && !OSSL_CMP_MSG_set_implicitConfirm(msg))
        goto oom;

    /* body */
    if ((resp = OSSL_CMP_CERTRESPONSE_new()) == NULL)
        goto oom;
    OSSL_CMP_PKISI_free(resp->status);
    if ((resp->status = OSSL_CMP_PKISI_dup(si)) == NULL ||
        !ASN1_INTEGER_set(resp->certReqId, certReqId)) {
        goto oom;
    }

    status = OSSL_CMP_PKISI_PKIStatus_get(resp->status);
    if (status != OSSL_CMP_PKISTATUS_rejection &&
        status != OSSL_CMP_PKISTATUS_waiting && cert != NULL) {
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

    if (bodytype == OSSL_CMP_PKIBODY_IP && caPubs != NULL &&
        (repMsg->caPubs = X509_chain_up_ref(caPubs)) == NULL)
        goto oom;
    if (chain != NULL &&
        !OSSL_CMP_sk_X509_add1_certs(msg->extraCerts, chain, 0, 1))
        goto oom;

    if (!(unprotectedErrors &&
          OSSL_CMP_PKISI_PKIStatus_get(si) == OSSL_CMP_PKISTATUS_rejection) &&
        !OSSL_CMP_MSG_protect(ctx, msg))
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

/*
 * Creates a new polling request PKIMessage for the given request ID
 * returns a pointer to the PKIMessage on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_pollReq_new(OSSL_CMP_CTX *ctx, int crid)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CMP_POLLREQ *preq = NULL;

    if (ctx == NULL ||
        (msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_POLLREQ)) == NULL)
        goto err;

    /* TODO: support multiple cert request IDs to poll */
    if ((preq = OSSL_CMP_POLLREQ_new()) == NULL ||
        !ASN1_INTEGER_set(preq->certReqId, crid) ||
        !sk_OSSL_CMP_POLLREQ_push(msg->body->value.pollReq, preq))
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

/*
 * Creates a new poll response message for the given request id
 * returns a poll response on success and NULL on error
 */
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

/*
 * Creates a new Revocation Request PKIMessage for ctx->oldCert based on
 * the settings in ctx.
 * returns a pointer to the PKIMessage on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_rr_new(OSSL_CMP_CTX *ctx)
{
    OSSL_CMP_MSG *msg = NULL;
    OSSL_CMP_REVDETAILS *rd = NULL;
    int ret;

    if (ctx == NULL || (ctx->oldCert == NULL && ctx->p10CSR == NULL)) {
        CMPerr(CMP_F_OSSL_CMP_RR_NEW, CMP_R_INVALID_ARGS);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_RR)) == NULL)
        goto err;

    if ((rd = OSSL_CMP_REVDETAILS_new()) == NULL)
        goto err;
    sk_OSSL_CMP_REVDETAILS_push(msg->body->value.rr, rd);

    /*
     * Fill the template from the contents of the certificate to be revoked;
     */
    ret = ctx->oldCert != NULL
    ? OSSL_CRMF_CERTTEMPLATE_fill(rd->certDetails,
                                  NULL /* pubkey would be redundant */,
                                  NULL /* subject would be redundant */,
                                  X509_get_issuer_name(ctx->oldCert),
                                  X509_get0_serialNumber(ctx->oldCert))
    : OSSL_CRMF_CERTTEMPLATE_fill(rd->certDetails,
                                  X509_REQ_get0_pubkey(ctx->p10CSR),
                                  X509_REQ_get_subject_name(ctx->p10CSR),
                                  NULL, NULL);
    if (ret == 0)
        goto err;

    /* revocation reason code is optional */
    if (ctx->revocationReason != CRL_REASON_NONE &&
        !add_crl_reason_extension(&rd->crlEntryDetails, ctx->revocationReason))
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

/*
 * Creates a revocation response message to a given revocation request.
 * Only handles the first given request. Consumes cid.
 */
OSSL_CMP_MSG *OSSL_CMP_rp_new(OSSL_CMP_CTX *ctx, OSSL_CMP_PKISI *si,
                              OSSL_CRMF_CERTID *cid, int unprot_err)
{
    OSSL_CMP_REVREPCONTENT *rep = NULL;
    OSSL_CMP_PKISI *si1 = NULL;
    OSSL_CMP_MSG *msg = NULL;

    if (ctx == NULL || si == NULL) {
        CMPerr(CMP_F_OSSL_CMP_RP_NEW, CMP_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((msg = OSSL_CMP_MSG_create(ctx, OSSL_CMP_PKIBODY_RP)) == NULL)
        goto oom;
    rep = msg->body->value.rp;

    if ((si1 = OSSL_CMP_PKISI_dup(si)) == NULL)
        goto oom;
    sk_OSSL_CMP_PKISI_push(rep->status, si1);

    if ((rep->certId = sk_OSSL_CRMF_CERTID_new_null()) == NULL)
        goto oom;
    if (cid != NULL) {
        if (!sk_OSSL_CRMF_CERTID_push(rep->certId, cid))
            goto oom;
        cid = NULL;
    }

    if (!(unprot_err &&
          OSSL_CMP_PKISI_PKIStatus_get(si) == OSSL_CMP_PKISTATUS_rejection) &&
        !OSSL_CMP_MSG_protect(ctx, msg))
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

/*
 * Creates a new Certificate Confirmation PKIMessage
 * returns a pointer to the PKIMessage on success, NULL on error
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
    /* set the # of the certReq */
    ASN1_INTEGER_set(certStatus->certReqId, OSSL_CMP_CERTREQID);
    /*
     * -- the hash of the certificate, using the same hash algorithm
     * -- as is used to create and verify the certificate signature
     */
    if (!CMP_CERTSTATUS_set_certHash(certStatus, ctx->newClCert))
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
        if (!OSSL_CMP_MSG_genm_items_push1(msg, ctx->genm_itavs))
            goto err;

    if (!OSSL_CMP_MSG_protect(ctx, msg))
        goto err;

    return msg;

 err:
    CMPerr(CMP_F_CMP_GEN_NEW, err_code);
    OSSL_CMP_MSG_free(msg);
    return NULL;
}

/*
 * Creates a new General Message with an empty itav stack
 * returns a pointer to the PKIMessage on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_genm_new(OSSL_CMP_CTX *ctx)
{
    return CMP_gen_new(ctx, OSSL_CMP_PKIBODY_GENM, CMP_R_ERROR_CREATING_GENM);
}

/*
 * Creates a new General Response with an empty itav stack
 * returns a pointer to the PKIMessage on success, NULL on error
 */
OSSL_CMP_MSG *OSSL_CMP_genp_new(OSSL_CMP_CTX *ctx)
{
    return CMP_gen_new(ctx, OSSL_CMP_PKIBODY_GENP, CMP_R_ERROR_CREATING_GENP);
}

/*
 * Creates Error Message with the given contents, copying si and errorDetails
 * returns a pointer to the PKIMessage on success, NULL on error
 */
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

OSSL_CMP_MSG *OSSL_CMP_MSG_load(const char *file)
{
    OSSL_CMP_MSG *msg = NULL;
    BIO *bio = NULL;

    if (file == NULL || (bio = BIO_new_file(file, "rb")) == NULL)
        return NULL;
    msg = d2i_OSSL_CMP_MSG_bio(bio, NULL);
    BIO_free(bio);
    return msg;
}

OSSL_CMP_MSG *d2i_OSSL_CMP_MSG_bio(BIO *bio, OSSL_CMP_MSG **msg)
{
    return ASN1_d2i_bio_of(OSSL_CMP_MSG, OSSL_CMP_MSG_new,
                           d2i_OSSL_CMP_MSG, bio, msg);
}

int i2d_OSSL_CMP_MSG_bio(BIO *bio, const OSSL_CMP_MSG *msg)
{
    return ASN1_i2d_bio_of(OSSL_CMP_MSG, i2d_OSSL_CMP_MSG, bio, msg);
}

OSSL_CMP_MSG *OSSL_CMP_MSG_read(const char *file)
{
    OSSL_CMP_MSG *msg = NULL;
    BIO *bio = NULL;

    if (file == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((bio = BIO_new_file(file, "rb")) == NULL)
        return NULL;
    msg = d2i_OSSL_CMP_MSG_bio(bio, NULL);
    BIO_free(bio);
    return msg;
}

int OSSL_CMP_MSG_write(const char *file, const OSSL_CMP_MSG *msg)
{
    BIO *bio;
    int res;

    if (file == NULL || msg == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return -1;
    }

    bio = BIO_new_file(file, "wb");
    if (bio == NULL)
        return -2;
    res = i2d_OSSL_CMP_MSG_bio(bio, msg);
    BIO_free(bio);
    return res;
}
