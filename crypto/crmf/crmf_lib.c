/*-
 * Copyright 2007-2021 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2018
 * Copyright Siemens AG 2015-2019
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * CRMF implementation by Martin Peylo, Miikka Viljanen, and David von Oheimb.
 */

/*
 * This file contains the functions that handle the individual items inside
 * the CRMF structures
 */

/*
 * NAMING
 *
 * The 0 functions use the supplied structure pointer directly in the parent and
 * it will be freed up when the parent is freed.
 *
 * The 1 functions use a copy of the supplied structure pointer (or in some
 * cases increases its link count) in the parent and so both should be freed up.
 */

#include <openssl/asn1t.h>

#include "crmf_local.h"
#include "internal/constant_time.h"
#include "internal/sizes.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/crmf.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
IMPLEMENT_ASN1_DUP_FUNCTION(X509_PUBKEY)
#endif

#if OPENSSL_VERSION_NUMBER < 0x30000000L
/*-
 * atyp = Attribute Type
 * valt = Value Type
 * ctrlinf = "regCtrl" or "regInfo"
 */
# define IMPLEMENT_CRMF_CTRL_FUNC(atyp, valt, ctrlinf)                        \
valt *OSSL_CRMF_MSG_get0_##ctrlinf##_##atyp(const OSSL_CRMF_MSG *msg)        \
{                                                                            \
    int i;                                                                   \
    STACK_OF(OSSL_CRMF_ATTRIBUTETYPEANDVALUE) *controls;                     \
    OSSL_CRMF_ATTRIBUTETYPEANDVALUE *atav = NULL;                            \
                                                                             \
    if (msg == NULL || msg->certReq == NULL)                                 \
        return NULL;                                                         \
    controls = msg->certReq->controls;                                       \
    for (i = 0; i < sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_num(controls); i++) { \
        atav = sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_value(controls, i);        \
        if (OBJ_obj2nid(atav->type) == NID_id_##ctrlinf##_##atyp)            \
            return atav->value.atyp;                                         \
    }                                                                        \
    return NULL;                                                             \
}                                                                            \
 \
int OSSL_CRMF_MSG_set1_##ctrlinf##_##atyp(OSSL_CRMF_MSG *msg, const valt *in) \
{                                                                         \
    OSSL_CRMF_ATTRIBUTETYPEANDVALUE *atav = NULL;                         \
                                                                          \
    if (msg == NULL || in == NULL)                                        \
        goto err;                                                         \
    if ((atav = OSSL_CRMF_ATTRIBUTETYPEANDVALUE_new()) == NULL)           \
        goto err;                                                         \
    if ((atav->type = OBJ_nid2obj(NID_id_##ctrlinf##_##atyp)) == NULL)    \
        goto err;                                                         \
    if ((atav->value.atyp = valt##_dup((valt *)in)) == NULL)              \
        goto err;                                                         \
    if (!OSSL_CRMF_MSG_push0_##ctrlinf(msg, atav))                        \
        goto err;                                                         \
    return 1;                                                             \
 err:                                                                     \
    OSSL_CRMF_ATTRIBUTETYPEANDVALUE_free(atav);                           \
    return 0;                                                             \
}

/*-
 * Pushes the given control attribute into the controls stack of a CertRequest
 * (section 6)
 * returns 1 on success, 0 on error
 */
static int OSSL_CRMF_MSG_push0_regCtrl(OSSL_CRMF_MSG *crm,
                                       OSSL_CRMF_ATTRIBUTETYPEANDVALUE *ctrl)
{
    int new = 0;

    if (crm == NULL || crm->certReq == NULL || ctrl == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (crm->certReq->controls == NULL) {
        crm->certReq->controls = sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_new_null();
        if (crm->certReq->controls == NULL)
            goto err;
        new = 1;
    }
    if (!sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_push(crm->certReq->controls, ctrl))
        goto err;

    return 1;
 err:
    if (new != 0) {
        sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_free(crm->certReq->controls);
        crm->certReq->controls = NULL;
    }
    return 0;
}

/* id-regCtrl-regToken Control (section 6.1) */
IMPLEMENT_CRMF_CTRL_FUNC(regToken, ASN1_STRING, regCtrl)

/* id-regCtrl-authenticator Control (section 6.2) */
# define ASN1_UTF8STRING_dup ASN1_STRING_dup
IMPLEMENT_CRMF_CTRL_FUNC(authenticator, ASN1_UTF8STRING, regCtrl)

int OSSL_CRMF_MSG_set0_SinglePubInfo(OSSL_CRMF_SINGLEPUBINFO *spi,
                                     int method, GENERAL_NAME *nm)
{
    if (spi == NULL
            || method < OSSL_CRMF_PUB_METHOD_DONTCARE
            || method > OSSL_CRMF_PUB_METHOD_LDAP) {
        ERR_raise(ERR_LIB_CRMF, ERR_R_PASSED_INVALID_ARGUMENT);
        return 0;
    }

    if (!ASN1_INTEGER_set(spi->pubMethod, method))
        return 0;
    GENERAL_NAME_free(spi->pubLocation);
    spi->pubLocation = nm;
    return 1;
}

int
OSSL_CRMF_MSG_PKIPublicationInfo_push0_SinglePubInfo(OSSL_CRMF_PKIPUBLICATIONINFO *pi,
                                                     OSSL_CRMF_SINGLEPUBINFO *spi)
{
    if (pi == NULL || spi == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }
    if (pi->pubInfos == NULL)
        pi->pubInfos = sk_OSSL_CRMF_SINGLEPUBINFO_new_null();
    if (pi->pubInfos == NULL)
        return 0;

    return sk_OSSL_CRMF_SINGLEPUBINFO_push(pi->pubInfos, spi);
}

int OSSL_CRMF_MSG_set_PKIPublicationInfo_action(OSSL_CRMF_PKIPUBLICATIONINFO *pi,
                                                int action)
{
    if (pi == NULL
            || action < OSSL_CRMF_PUB_ACTION_DONTPUBLISH
            || action > OSSL_CRMF_PUB_ACTION_PLEASEPUBLISH) {
        ERR_raise(ERR_LIB_CRMF, ERR_R_PASSED_INVALID_ARGUMENT);
        return 0;
    }

    return ASN1_INTEGER_set(pi->action, action);
}

/* id-regCtrl-pkiPublicationInfo Control (section 6.3) */
IMPLEMENT_CRMF_CTRL_FUNC(pkiPublicationInfo, OSSL_CRMF_PKIPUBLICATIONINFO,
                         regCtrl)

/* id-regCtrl-oldCertID Control (section 6.5) from the given */
IMPLEMENT_CRMF_CTRL_FUNC(oldCertID, OSSL_CRMF_CERTID, regCtrl)

OSSL_CRMF_CERTID *OSSL_CRMF_CERTID_gen(const X509_NAME *issuer,
                                       const ASN1_INTEGER *serial)
{
    OSSL_CRMF_CERTID *cid = NULL;

    if (issuer == NULL || serial == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((cid = OSSL_CRMF_CERTID_new()) == NULL)
        goto err;

    if (!X509_NAME_set(&cid->issuer->d.directoryName, issuer))
        goto err;
    cid->issuer->type = GEN_DIRNAME;

    ASN1_INTEGER_free(cid->serialNumber);
    if ((cid->serialNumber = ASN1_INTEGER_dup(serial)) == NULL)
        goto err;

    return cid;

 err:
    OSSL_CRMF_CERTID_free(cid);
    return NULL;
}

/*
 * id-regCtrl-protocolEncrKey Control (section 6.6)
 */
IMPLEMENT_CRMF_CTRL_FUNC(protocolEncrKey, X509_PUBKEY, regCtrl)

/*-
 * Pushes the attribute given in regInfo in to the CertReqMsg->regInfo stack.
 * (section 7)
 * returns 1 on success, 0 on error
 */
static int OSSL_CRMF_MSG_push0_regInfo(OSSL_CRMF_MSG *crm,
                                       OSSL_CRMF_ATTRIBUTETYPEANDVALUE *ri)
{
    STACK_OF(OSSL_CRMF_ATTRIBUTETYPEANDVALUE) *info = NULL;

    if (crm == NULL || ri == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (crm->regInfo == NULL)
        crm->regInfo = info = sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_new_null();
    if (crm->regInfo == NULL)
        goto err;
    if (!sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_push(crm->regInfo, ri))
        goto err;
    return 1;

 err:
    if (info != NULL)
        crm->regInfo = NULL;
    sk_OSSL_CRMF_ATTRIBUTETYPEANDVALUE_free(info);
    return 0;
}

/* id-regInfo-utf8Pairs to regInfo (section 7.1) */
IMPLEMENT_CRMF_CTRL_FUNC(utf8Pairs, ASN1_UTF8STRING, regInfo)

/* id-regInfo-certReq to regInfo (section 7.2) */
IMPLEMENT_CRMF_CTRL_FUNC(certReq, OSSL_CRMF_CERTREQUEST, regInfo)

/* retrieves the certificate template of crm */
OSSL_CRMF_CERTTEMPLATE *OSSL_CRMF_MSG_get0_tmpl(const OSSL_CRMF_MSG *crm)
{
    if (crm == NULL || crm->certReq == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return NULL;
    }
    return crm->certReq->certTemplate;
}

int OSSL_CRMF_MSG_set0_validity(OSSL_CRMF_MSG *crm,
                                ASN1_TIME *notBefore, ASN1_TIME *notAfter)
{
    OSSL_CRMF_OPTIONALVALIDITY *vld;
    OSSL_CRMF_CERTTEMPLATE *tmpl = OSSL_CRMF_MSG_get0_tmpl(crm);

    if (tmpl == NULL) { /* also crm == NULL implies this */
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if ((vld = OSSL_CRMF_OPTIONALVALIDITY_new()) == NULL)
        return 0;
    vld->notBefore = notBefore;
    vld->notAfter = notAfter;
    tmpl->validity = vld;
    return 1;
}

int OSSL_CRMF_MSG_set_certReqId(OSSL_CRMF_MSG *crm, int rid)
{
    if (crm == NULL || crm->certReq == NULL || crm->certReq->certReqId == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    return ASN1_INTEGER_set(crm->certReq->certReqId, rid);
}

/* get ASN.1 encoded integer, return -1 on error */
static int crmf_asn1_get_int(const ASN1_INTEGER *a)
{
    int64_t res;

    if (!ASN1_INTEGER_get_int64(&res, a)) {
        ERR_raise(ERR_LIB_CRMF, ASN1_R_INVALID_NUMBER);
        return -1;
    }
    if (res < INT_MIN) {
        ERR_raise(ERR_LIB_CRMF, ASN1_R_TOO_SMALL);
        return -1;
    }
    if (res > INT_MAX) {
        ERR_raise(ERR_LIB_CRMF, ASN1_R_TOO_LARGE);
        return -1;
    }
    return (int)res;
}

int OSSL_CRMF_MSG_get_certReqId(const OSSL_CRMF_MSG *crm)
{
    if (crm == NULL || /* not really needed: */ crm->certReq == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return -1;
    }
    return crmf_asn1_get_int(crm->certReq->certReqId);
}

int OSSL_CRMF_MSG_set0_extensions(OSSL_CRMF_MSG *crm,
                                  X509_EXTENSIONS *exts)
{
    OSSL_CRMF_CERTTEMPLATE *tmpl = OSSL_CRMF_MSG_get0_tmpl(crm);

    if (tmpl == NULL) { /* also crm == NULL implies this */
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (sk_X509_EXTENSION_num(exts) == 0) {
        sk_X509_EXTENSION_free(exts);
        exts = NULL; /* do not include empty extensions list */
    }

    sk_X509_EXTENSION_pop_free(tmpl->extensions, X509_EXTENSION_free);
    tmpl->extensions = exts;
    return 1;
}

int OSSL_CRMF_MSG_push0_extension(OSSL_CRMF_MSG *crm,
                                  X509_EXTENSION *ext)
{
    int new = 0;
    OSSL_CRMF_CERTTEMPLATE *tmpl = OSSL_CRMF_MSG_get0_tmpl(crm);

    if (tmpl == NULL || ext == NULL) { /* also crm == NULL implies this */
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (tmpl->extensions == NULL) {
        if ((tmpl->extensions = sk_X509_EXTENSION_new_null()) == NULL)
            goto err;
        new = 1;
    }

    if (!sk_X509_EXTENSION_push(tmpl->extensions, ext))
        goto err;
    return 1;
 err:
    if (new != 0) {
        sk_X509_EXTENSION_free(tmpl->extensions);
        tmpl->extensions = NULL;
    }
    return 0;
}

static int create_popo_signature(OSSL_CRMF_POPOSIGNINGKEY *ps,
                                 const OSSL_CRMF_CERTREQUEST *cr,
                                 EVP_PKEY *pkey, const EVP_MD *digest,
                                 OSSL_LIB_CTX *libctx, const char *propq)
{
    char name[80] = "";

    if (ps == NULL || cr == NULL || pkey == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }
    if (ps->poposkInput != NULL) {
        /* We do not support cases 1+2 defined in RFC 4211, section 4.1 */
        ERR_raise(ERR_LIB_CRMF, CRMF_R_POPOSKINPUT_NOT_SUPPORTED);
        return 0;
    }

    if (EVP_PKEY_get_default_digest_name(pkey, name, sizeof(name)) > 0
            && strcmp(name, "UNDEF") == 0) /* at least for Ed25519, Ed448 */
        digest = NULL;

    return ASN1_item_sign_ex(ASN1_ITEM_rptr(OSSL_CRMF_CERTREQUEST),
                             ps->algorithmIdentifier, NULL, ps->signature, cr,
                             NULL, pkey, digest, libctx, propq);
}

int OSSL_CRMF_MSG_create_popo(int meth, OSSL_CRMF_MSG *crm,
                              EVP_PKEY *pkey, const EVP_MD *digest,
                              OSSL_LIB_CTX *libctx, const char *propq)
{
    OSSL_CRMF_POPO *pp = NULL;
    ASN1_INTEGER *tag = NULL;

    if (crm == NULL || (meth == OSSL_CRMF_POPO_SIGNATURE && pkey == NULL)) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (meth == OSSL_CRMF_POPO_NONE)
        goto end;
    if ((pp = OSSL_CRMF_POPO_new()) == NULL)
        goto err;
    pp->type = meth;

    switch (meth) {
    case OSSL_CRMF_POPO_RAVERIFIED:
        if ((pp->value.raVerified = ASN1_NULL_new()) == NULL)
            goto err;
        break;

    case OSSL_CRMF_POPO_SIGNATURE:
        {
            OSSL_CRMF_POPOSIGNINGKEY *ps = OSSL_CRMF_POPOSIGNINGKEY_new();

            if (ps == NULL)
                goto err;
            if (!create_popo_signature(ps, crm->certReq, pkey, digest,
                                       libctx, propq)) {
                OSSL_CRMF_POPOSIGNINGKEY_free(ps);
                goto err;
            }
            pp->value.signature = ps;
        }
        break;

    case OSSL_CRMF_POPO_KEYENC:
        if ((pp->value.keyEncipherment = OSSL_CRMF_POPOPRIVKEY_new()) == NULL)
            goto err;
        tag = ASN1_INTEGER_new();
        pp->value.keyEncipherment->type =
            OSSL_CRMF_POPOPRIVKEY_SUBSEQUENTMESSAGE;
        pp->value.keyEncipherment->value.subsequentMessage = tag;
        if (tag == NULL
                || !ASN1_INTEGER_set(tag, OSSL_CRMF_SUBSEQUENTMESSAGE_ENCRCERT))
            goto err;
        break;

    default:
        ERR_raise(ERR_LIB_CRMF, CRMF_R_UNSUPPORTED_METHOD_FOR_CREATING_POPO);
        goto err;
    }

 end:
    OSSL_CRMF_POPO_free(crm->popo);
    crm->popo = pp;

    return 1;
 err:
    OSSL_CRMF_POPO_free(pp);
    return 0;
}

/* verifies the Proof-of-Possession of the request with the given rid in reqs */
int OSSL_CRMF_MSGS_verify_popo(const OSSL_CRMF_MSGS *reqs,
                               int rid, int acceptRAVerified,
                               OSSL_LIB_CTX *libctx, const char *propq)
{
    OSSL_CRMF_MSG *req = NULL;
    X509_PUBKEY *pubkey = NULL;
    OSSL_CRMF_POPOSIGNINGKEY *sig = NULL;
    const ASN1_ITEM *it;
    void *asn;

    if (reqs == NULL || (req = sk_OSSL_CRMF_MSG_value(reqs, rid)) == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }

    if (req->popo == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_MISSING);
        return 0;
    }

    switch (req->popo->type) {
    case OSSL_CRMF_POPO_RAVERIFIED:
        if (!acceptRAVerified) {
            ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_RAVERIFIED_NOT_ACCEPTED);
            return 0;
        }
        break;
    case OSSL_CRMF_POPO_SIGNATURE:
        pubkey = req->certReq->certTemplate->publicKey;
        if (pubkey == NULL) {
            ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_MISSING_PUBLIC_KEY);
            return 0;
        }
        sig = req->popo->value.signature;
        if (sig->poposkInput != NULL) {
            /*
             * According to RFC 4211: publicKey contains a copy of
             * the public key from the certificate template. This MUST be
             * exactly the same value as contained in the certificate template.
             */
            if (sig->poposkInput->publicKey == NULL) {
                ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_MISSING_PUBLIC_KEY);
                return 0;
            }
            if (X509_PUBKEY_eq(pubkey, sig->poposkInput->publicKey) != 1) {
                ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_INCONSISTENT_PUBLIC_KEY);
                return 0;
            }
            /*
             * At this point, may check the contents of the authInfo sub-field,
             * see RFC 4211 https://tools.ietf.org/html/rfc4211#section-4.1
             */
            it = ASN1_ITEM_rptr(OSSL_CRMF_POPOSIGNINGKEYINPUT);
            asn = sig->poposkInput;
        } else {
            if (req->certReq->certTemplate->subject == NULL) {
                ERR_raise(ERR_LIB_CRMF, CRMF_R_POPO_MISSING_SUBJECT);
                return 0;
            }
            it = ASN1_ITEM_rptr(OSSL_CRMF_CERTREQUEST);
            asn = req->certReq;
        }
        if (ASN1_item_verify_ex(it, sig->algorithmIdentifier, sig->signature,
                                asn, NULL, X509_PUBKEY_get0(pubkey), libctx,
                                propq) < 1)
            return 0;
        break;
    case OSSL_CRMF_POPO_KEYENC:
        /*
         * When OSSL_CMP_certrep_new() supports encrypted certs,
         * should return 1 if the type of req->popo->value.keyEncipherment
         * is OSSL_CRMF_POPOPRIVKEY_SUBSEQUENTMESSAGE and
         * its value.subsequentMessage == OSSL_CRMF_SUBSEQUENTMESSAGE_ENCRCERT
         */
    case OSSL_CRMF_POPO_KEYAGREE:
    default:
        ERR_raise(ERR_LIB_CRMF, CRMF_R_UNSUPPORTED_POPO_METHOD);
        return 0;
    }
    return 1;
}

/* retrieves the serialNumber of the given cert template or NULL on error */
const ASN1_INTEGER
*OSSL_CRMF_CERTTEMPLATE_get0_serialNumber(const OSSL_CRMF_CERTTEMPLATE *tmpl)
{
    return tmpl != NULL ? tmpl->serialNumber : NULL;
}

const X509_NAME
*OSSL_CRMF_CERTTEMPLATE_get0_subject(const OSSL_CRMF_CERTTEMPLATE *tmpl)
{
    return tmpl != NULL ? tmpl->subject : NULL;
}

/* retrieves the issuer name of the given cert template or NULL on error */
const X509_NAME
*OSSL_CRMF_CERTTEMPLATE_get0_issuer(const OSSL_CRMF_CERTTEMPLATE *tmpl)
{
    return tmpl != NULL ? tmpl->issuer : NULL;
}

X509_EXTENSIONS
*OSSL_CRMF_CERTTEMPLATE_get0_extensions(const OSSL_CRMF_CERTTEMPLATE *tmpl)
{
    return tmpl != NULL ? tmpl->extensions : NULL;
}

/* retrieves the issuer name of the given CertId or NULL on error */
const X509_NAME *OSSL_CRMF_CERTID_get0_issuer(const OSSL_CRMF_CERTID *cid)
{
    return cid != NULL && cid->issuer->type == GEN_DIRNAME ?
        cid->issuer->d.directoryName : NULL;
}

/* retrieves the serialNumber of the given CertId or NULL on error */
const ASN1_INTEGER *OSSL_CRMF_CERTID_get0_serialNumber(const OSSL_CRMF_CERTID *cid)
{
    return cid != NULL ? cid->serialNumber : NULL;
}

# if OPENSSL_VERSION_NUMBER < 0x30100000L
/* copied from ../x509/x_pubkey.c: */
struct X509_pubkey_st {
    X509_ALGOR *algor;
    ASN1_BIT_STRING *public_key;
    EVP_PKEY *pkey;
};
/* Added to OpenSSL 3.1 in #18668 */
static void X509_PUBKEY_set0_public_key(X509_PUBKEY *pub,
                                        unsigned char *penc, int penclen)
{
    OPENSSL_free(pub->public_key->data);
    pub->public_key->data = penc;
    pub->public_key->length = penclen;
    /* Set number of unused bits to zero */
    pub->public_key->flags &= ~(ASN1_STRING_FLAG_BITS_LEFT | 0x07);
    pub->public_key->flags |= ASN1_STRING_FLAG_BITS_LEFT;
}
# endif

/*-
 * fill in certificate template.
 * Any value argument that is NULL will leave the respective field unchanged.
 */
int OSSL_CRMF_CERTTEMPLATE_fill(OSSL_CRMF_CERTTEMPLATE *tmpl,
                                EVP_PKEY *pubkey, int central_keygen,
                                const X509_NAME *subject,
                                const X509_NAME *issuer,
                                const ASN1_INTEGER *serial)
{
    if (tmpl == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return 0;
    }
    if (subject != NULL && !X509_NAME_set((X509_NAME **)&tmpl->subject, subject))
        return 0;
    if (issuer != NULL && !X509_NAME_set((X509_NAME **)&tmpl->issuer, issuer))
        return 0;
    if (serial != NULL) {
        ASN1_INTEGER_free(tmpl->serialNumber);
        if ((tmpl->serialNumber = ASN1_INTEGER_dup(serial)) == NULL)
            return 0;
    }
    if (pubkey != NULL && !X509_PUBKEY_set(&tmpl->publicKey, pubkey))
        return 0;
    if (pubkey != NULL && central_keygen) {
        /* set public_key bit string to NULL */
        X509_PUBKEY_set0_public_key(tmpl->publicKey, NULL, 0);
    }
    return 1;
}


# ifndef OPENSSL_NO_CMS
/* check for KGA authorization implied by CA flag or by explicit EKU cmKGA */
static int check_cmKGA(ossl_unused const X509_PURPOSE *purpose,
                       const X509 *x, int ca)
{
    STACK_OF(ASN1_OBJECT) *ekus;
    int i, ret = 1;

    if (ca)
        return ret;
    ekus =  X509_get_ext_d2i(x, NID_ext_key_usage, NULL, NULL);
    for (i = 0; i < sk_ASN1_OBJECT_num(ekus); i++) {
#  if OPENSSL_VERSION_NUMBER >= 0x30000000L
        if (OBJ_obj2nid(sk_ASN1_OBJECT_value(ekus, i)) == NID_cmKGA)
            goto end;
#  else
        {
            char txt[100];

            if (OBJ_obj2txt(txt, sizeof(txt), sk_ASN1_OBJECT_value(ekus, i), 1)
                    && strcmp(txt, "1.3.6.1.5.5.7.3.32") == 0) /* OID cmKGA */
                goto end;
        }
#  endif
    }
    ret = 0;

 end:
    sk_ASN1_OBJECT_pop_free(ekus, ASN1_OBJECT_free);
    return ret;
}

DECLARE_ASN1_ITEM(CMS_SignedData) /* copied from cms_local.h */

/* added to OpenSSL 3.1 in #18301 */
BIO *CMS_EnvelopedData_decrypt(CMS_EnvelopedData *env, BIO *detached_data,
                               EVP_PKEY *pkey, X509 *cert,
                               ASN1_OCTET_STRING *secret, unsigned int flags,
                               OSSL_LIB_CTX *libctx, const char *propq);
/* added to OpenSSL 3.1 in #18667 */
IMPLEMENT_ASN1_ALLOC_FUNCTIONS(CMS_SignedData)
BIO *CMS_SignedData_verify(CMS_SignedData *sd, BIO *detached_data,
                           STACK_OF(X509) *scerts, X509_STORE *store,
                           STACK_OF(X509) *extra, STACK_OF(X509_CRL) *crls,
                           unsigned int flags,
                           OSSL_LIB_CTX *libctx, const char *propq);
# endif /* OPENSSL_NO_CMS */

EVP_PKEY
*OSSL_CRMF_ENCRYPTEDKEY_get1_pkey(OSSL_CRMF_ENCRYPTEDKEY *encryptedKey,
                                  X509_STORE *ts, STACK_OF(X509) *extra,
                                  EVP_PKEY *pkey, X509 *cert,
                                  ASN1_OCTET_STRING *secret,
                                  OSSL_LIB_CTX *libctx, const char *propq)
{
# ifndef OPENSSL_NO_CMS
    BIO *bio = NULL;
    CMS_SignedData *sd = NULL;
    BIO *pkey_bio = NULL;
    int purpose_id = X509_PURPOSE_get_count() + 1;
# endif
    EVP_PKEY *ret = NULL;

    if (encryptedKey == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return NULL;
    }
    if (encryptedKey->type != OSSL_CRMF_ENCRYPTEDKEY_ENVELOPEDDATA) {
        unsigned char *p = NULL;
        const unsigned char *p_copy;
        int len;

        p = OSSL_CRMF_ENCRYPTEDVALUE_decrypt(encryptedKey->value.encryptedValue,
                                             pkey, &len, libctx, propq);
        if ((p_copy = p) != NULL)
            ret = d2i_AutoPrivateKey_ex(NULL, &p_copy, len, libctx, propq);
        OPENSSL_free(p);
        return ret;
    }

# ifndef OPENSSL_NO_CMS
    if ((bio = CMS_EnvelopedData_decrypt(encryptedKey->value.envelopedData,
                                         NULL, pkey, cert, secret, 0,
                                         libctx, propq)) == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECRYPTING_ENCRYPTEDKEY);
        goto end;
    }

    if (!X509_PURPOSE_add(purpose_id, X509_TRUST_COMPAT, 0, check_cmKGA,
                          "CMP Key Generation Authority", "cmKGA", NULL)
            || !X509_STORE_set_purpose(ts, purpose_id)) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_SETTING_PURPOSE);
        goto end;
    }
    sd =  ASN1_item_d2i_bio(ASN1_ITEM_rptr(CMS_SignedData), bio, NULL);
    extra = NULL; /* workaround for CMS_add0_cert() in cms_lib.c not allowing duplicate untrusted certs */
    if (sd == NULL
        || (pkey_bio = CMS_SignedData_verify(sd, NULL, NULL /* scerts */, ts,
                                             extra, NULL, 0, libctx, propq))
        == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_VERIFYING_ENCRYPTEDKEY);
        goto end;
    }

    /* unpack AsymmetricKeyPackage */
    ret = d2i_PrivateKey_ex_bio(pkey_bio, NULL, libctx, propq);
    if (ret == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECODING_ENCRYPTEDKEY);
        goto end;
    }

 end:
    CMS_SignedData_free(sd);
    BIO_free(bio);
    BIO_free(pkey_bio);
    return ret;
# else
    /* prevent warning on unused parameters: */
    ((void)ts, (void)extra, (void)cert, (void)secret);
    ERR_raise(ERR_LIB_CRMF, CRMF_R_CMS_NOT_SUPPORTED);
    return NULL;
# endif /* OPENSSL_NO_CMS */
}

unsigned char
*OSSL_CRMF_ENCRYPTEDVALUE_decrypt(const OSSL_CRMF_ENCRYPTEDVALUE *enc,
                                  EVP_PKEY *pkey, int *outlen,
                                  OSSL_LIB_CTX *libctx, const char *propq)
{
    EVP_CIPHER_CTX *evp_ctx = NULL; /* context for symmetric encryption */
    unsigned char *ek = NULL; /* decrypted symmetric encryption key */
    size_t eksize = 0; /* size of decrypted symmetric encryption key */
    const EVP_CIPHER *cipher = NULL; /* used cipher */
    int cikeysize = 0; /* key size from cipher */
    unsigned char *iv = NULL; /* initial vector for symmetric encryption */
    unsigned char *out = NULL; /* decryption output buffer */
    int symmAlg = 0; /* NIDs for symmetric algorithm */
    int n, ret = 0;
    EVP_PKEY_CTX *pkctx = NULL; /* private key context */

    if (outlen == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return NULL;
    }
    *outlen = 0;
    if (enc == NULL || enc->symmAlg == NULL || enc->encSymmKey == NULL
            || enc->encValue == NULL || pkey == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_NULL_ARGUMENT);
        return NULL;
    }

    if ((symmAlg = OBJ_obj2nid(enc->symmAlg->algorithm)) == 0) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_UNSUPPORTED_CIPHER);
        return NULL;
    }

    /* select symmetric cipher based on algorithm given in message */
    if ((cipher = EVP_get_cipherbynid(symmAlg)) == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_UNSUPPORTED_CIPHER);
        goto end;
    }

    cikeysize = EVP_CIPHER_get_key_length(cipher);
    /* first the symmetric key needs to be decrypted */
    pkctx = EVP_PKEY_CTX_new_from_pkey(libctx, pkey, propq);
    if (pkctx != NULL && EVP_PKEY_decrypt_init(pkctx)) {
        ASN1_BIT_STRING *encKey = enc->encSymmKey;
        size_t failure;
        int retval;

        if (EVP_PKEY_decrypt(pkctx, NULL, &eksize,
                             encKey->data, encKey->length) <= 0
                || (ek = OPENSSL_malloc(eksize)) == NULL)
            goto end;
        retval = EVP_PKEY_decrypt(pkctx, ek, &eksize,
                                  encKey->data, encKey->length);
        ERR_clear_error(); /* error state may have sensitive information */
        failure = ~constant_time_is_zero_s(constant_time_msb(retval)
                                           | constant_time_is_zero(retval));
        failure |= ~constant_time_eq_s(eksize, (size_t)cikeysize);
        if (failure) {
            ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECRYPTING_SYMMETRIC_KEY);
            goto end;
        }
    } else {
        goto end;
    }
    if ((iv = OPENSSL_malloc(EVP_CIPHER_get_iv_length(cipher))) == NULL)
        goto end;
    if (ASN1_TYPE_get_octetstring(enc->symmAlg->parameter, iv,
                                  EVP_CIPHER_get_iv_length(cipher))
        != EVP_CIPHER_get_iv_length(cipher)) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_MALFORMED_IV);
        goto end;
    }

    if ((out = OPENSSL_malloc(enc->encValue->length +
                              EVP_CIPHER_get_block_size(cipher))) == NULL
            || (evp_ctx = EVP_CIPHER_CTX_new()) == NULL)
        goto end;
    EVP_CIPHER_CTX_set_padding(evp_ctx, 0);

    if (!EVP_DecryptInit(evp_ctx, cipher, ek, iv)
            || !EVP_DecryptUpdate(evp_ctx, out, outlen,
                                  enc->encValue->data,
                                  enc->encValue->length)
            || !EVP_DecryptFinal(evp_ctx, out + *outlen, &n)) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECRYPTING_ENCRYPTEDVALUE);
        goto end;
    }
    *outlen += n;
    ret = 1;

 end:
    EVP_PKEY_CTX_free(pkctx);
    EVP_CIPHER_CTX_free(evp_ctx);
    OPENSSL_clear_free(ek, eksize);
    OPENSSL_free(iv);
    if (ret)
        return out;
    OPENSSL_free(out);
    return NULL;
}

/*-
 * Decrypts the certificate in the given encryptedValue using private key pkey.
 * This is needed for the indirect PoP method as in RFC 4210 section 5.2.8.2.
 *
 * returns a pointer to the decrypted certificate
 * returns NULL on error or if no certificate available
 */
X509
*OSSL_CRMF_ENCRYPTEDVALUE_get1_encCert(const OSSL_CRMF_ENCRYPTEDVALUE *ecert,
                                       OSSL_LIB_CTX *libctx, const char *propq,
                                       EVP_PKEY *pkey)
{
    unsigned char *buf = NULL;
    const unsigned char *p;
    int len;
    X509 *cert = NULL;

    buf = OSSL_CRMF_ENCRYPTEDVALUE_decrypt(ecert, pkey, &len, libctx, propq);
    if ((p = buf) == NULL
            || (cert = X509_new_ex(libctx, propq)) == NULL)
        goto end;
    if (d2i_X509(&cert, &p, len) == NULL) {
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECODING_CERTIFICATE);
        X509_free(cert);
        cert = NULL;
    }

 end:
    OPENSSL_free(buf);
    return cert;
}

/*-
 * Decrypts the certificate in the given encryptedKey using private key pkey.
 * This is needed for the indirect PoP method as in RFC 4210 section 5.2.8.2.
 *
 * returns a pointer to the decrypted certificate
 * returns NULL on error or if no certificate available
 */
X509
*OSSL_CRMF_ENCRYPTEDKEY_get1_encCert(const OSSL_CRMF_ENCRYPTEDKEY *ecert,
                                     OSSL_LIB_CTX *libctx, const char *propq,
                                     EVP_PKEY *pkey, unsigned int flags)
{
# ifndef OPENSSL_NO_CMS
    BIO *bio;
    X509 *cert = NULL;
# endif

    if (ecert->type != OSSL_CRMF_ENCRYPTEDKEY_ENVELOPEDDATA)
        return OSSL_CRMF_ENCRYPTEDVALUE_get1_encCert(ecert->value.encryptedValue,
                                                     libctx, propq, pkey);
# ifndef OPENSSL_NO_CMS
    bio = CMS_EnvelopedData_decrypt(ecert->value.envelopedData, NULL,
                                    pkey, NULL /* cert */, NULL, flags,
                                    libctx, propq);
    if (bio == NULL)
        return NULL;
    cert = d2i_X509_bio(bio, NULL);
    if (cert == NULL)
        ERR_raise(ERR_LIB_CRMF, CRMF_R_ERROR_DECODING_CERTIFICATE);
    BIO_free(bio);
    return cert;
# else
    (void)flags; /* prevent warning on unused parameter */
    ERR_raise(ERR_LIB_CRMF, CRMF_R_CMS_NOT_SUPPORTED);
    return NULL;
# endif /* OPENSSL_NO_CMS */
}


# ifndef OPENSSL_NO_CMS

#  if OPENSSL_VERSION_NUMBER < 0x30100000L

/* copied from crypto/cms/cms_local.h for being able to access */
typedef struct CMS_DigestedData_st CMS_DigestedData;
typedef struct CMS_EncryptedData_st CMS_EncryptedData;
typedef struct CMS_AuthenticatedData_st CMS_AuthenticatedData;
typedef struct CMS_AuthEnvelopedData_st CMS_AuthEnvelopedData;
typedef struct CMS_CompressedData_st CMS_CompressedData;
typedef struct CMS_CTX_st CMS_CTX;
struct CMS_CTX_st {
    OSSL_LIB_CTX *libctx;
    char *propq;
};
struct CMS_ContentInfo_st {
    ASN1_OBJECT *contentType;
    union {
        ASN1_OCTET_STRING *data;
        CMS_SignedData *signedData;
        CMS_EnvelopedData *envelopedData;
        CMS_DigestedData *digestedData;
        CMS_EncryptedData *encryptedData;
        CMS_AuthEnvelopedData *authEnvelopedData;
        CMS_AuthenticatedData *authenticatedData;
        CMS_CompressedData *compressedData;
        ASN1_TYPE *other;
        /* Other types ... */
        void *otherData;
    } d;
    CMS_CTX ctx;
};


/* added to OpenSSL 3.1 in #18301 */
BIO *CMS_EnvelopedData_decrypt(CMS_EnvelopedData *env, BIO *detached_data,
                               EVP_PKEY *pkey, X509 *cert,
                               ASN1_OCTET_STRING *secret, unsigned int flags,
                               OSSL_LIB_CTX *libctx, const char *propq)
{
    CMS_ContentInfo *ci;
    BIO *bio = NULL;
    int res = 0;

    if (env == NULL) {
        ERR_raise(ERR_LIB_CMS, ERR_R_PASSED_NULL_PARAMETER);
        return NULL;
    }

    if ((ci = CMS_ContentInfo_new_ex(libctx, propq)) == NULL
            || (bio = BIO_new(BIO_s_mem())) == NULL)
        goto end;
    ci->contentType = OBJ_nid2obj(NID_pkcs7_enveloped);
    ci->d.envelopedData = env;
    if (secret != NULL
        && CMS_decrypt_set1_password(ci, (unsigned char *)
                                     ASN1_STRING_get0_data(secret),
                                     ASN1_STRING_length(secret)) != 1)
        goto end;
    res = CMS_decrypt(ci, pkey, cert, detached_data, bio, flags);

 end:
    if (ci != NULL)
        ci->d.envelopedData = NULL; /* do not indirectly free |env| */
    CMS_ContentInfo_free(ci);
    if (!res) {
        BIO_free(bio);
        bio = NULL;
    }
    return bio;
}

/* added to OpenSSL 3.1 in #18667 */
BIO *CMS_SignedData_verify(CMS_SignedData *sd, BIO *detached_data,
                           STACK_OF(X509) *scerts, X509_STORE *store,
                           STACK_OF(X509) *extra, STACK_OF(X509_CRL) *crls,
                           unsigned int flags,
                           OSSL_LIB_CTX *libctx, const char *propq)
{
    CMS_ContentInfo *ci;
    BIO *bio = NULL;
    int i, res = 0;

    if (sd == NULL) {
        ERR_raise(ERR_LIB_CMS, ERR_R_PASSED_NULL_PARAMETER);
        return NULL;
    }

    if ((ci = CMS_ContentInfo_new_ex(libctx, propq)) == NULL)
        return NULL;
    if ((bio = BIO_new(BIO_s_mem())) == NULL)
        goto end;
    ci->contentType = OBJ_nid2obj(NID_pkcs7_signed);
    ci->d.signedData = sd;

    for (i = 0; i < sk_X509_num(extra); i++)
        if (!CMS_add1_cert(ci, sk_X509_value(extra, i)))
            goto end;
    for (i = 0; i < sk_X509_CRL_num(crls); i++)
        if (!CMS_add1_crl(ci, sk_X509_CRL_value(crls, i)))
            goto end;
    res = CMS_verify(ci, scerts, store, detached_data, bio, flags);

 end:
    if (ci != NULL)
        ci->d.signedData = NULL; /* do not indirectly free |sd| */
    CMS_ContentInfo_free(ci);
    if (!res) {
        BIO_free(bio);
        bio = NULL;
    }
    return bio;
}

#  endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */
# endif /* OPENSSL_NO_CMS */
#endif /* OPENSSL_VERSION_NUMBER < 0x30100000L */
