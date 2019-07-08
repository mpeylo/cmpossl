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

/* CMP functions for producing msg protection including filling in extraCerts */

#include "cmp_int.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <openssl/asn1t.h>
#include <openssl/cmp.h>
#include <openssl/crmf.h>
#include <openssl/err.h>
#include <openssl/x509.h>

/*
 * This function is also used for verification from cmp_vfy.
 *
 * Calculate protection for given PKImessage utilizing the given credentials
 * and the algorithm parameters set inside the message header's protectionAlg.
 *
 * Either secret or pkey must be set, the other must be NULL. Attempts doing
 * PBMAC in case 'secret' is set and signature if 'pkey' is set - but will only
 * do the protection already marked in msg->header->protectionAlg.
 *
 * returns ptr to ASN1_BIT_STRING containing protection on success, else NULL
 */
ASN1_BIT_STRING *CMP_calc_protection(const OSSL_CMP_MSG *msg,
                                     const ASN1_OCTET_STRING *secret,
                                     EVP_PKEY *pkey)
{
    ASN1_BIT_STRING *prot = NULL;
    CMP_PROTECTEDPART prot_part;
    const ASN1_OBJECT *algorOID = NULL;

    int l;
    size_t prot_part_der_len;
    unsigned char *prot_part_der = NULL;
    size_t sig_len;
    unsigned char *protection = NULL;

    const void *ppval = NULL;
    int pptype = 0;

    OSSL_CRMF_PBMPARAMETER *pbm = NULL;
    ASN1_STRING *pbm_str = NULL;
    const unsigned char *pbm_str_uc = NULL;

    EVP_MD_CTX *evp_ctx = NULL;
    int md_NID;
    const EVP_MD *md = NULL;

    /* construct data to be signed */
    prot_part.header = msg->header;
    prot_part.body = msg->body;

    l = i2d_CMP_PROTECTEDPART(&prot_part, &prot_part_der);
    if (l < 0 || prot_part_der == NULL)
        goto err;
    prot_part_der_len = (size_t) l;

    X509_ALGOR_get0(&algorOID, &pptype, &ppval, msg->header->protectionAlg);

    if (secret != NULL && pkey == NULL) {
        if (NID_id_PasswordBasedMAC == OBJ_obj2nid(algorOID)) {
            if (ppval == NULL)
                goto err;

            pbm_str = (ASN1_STRING *)ppval;
            pbm_str_uc = pbm_str->data;
            pbm = d2i_OSSL_CRMF_PBMPARAMETER(NULL, &pbm_str_uc, pbm_str->length);

            if (!(OSSL_CRMF_pbm_new(pbm, prot_part_der, prot_part_der_len,
                                    secret->data, secret->length,
                                    &protection, &sig_len)))
                goto err;
        } else {
            CMPerr(CMP_F_CMP_CALC_PROTECTION, CMP_R_WRONG_ALGORITHM_OID);
            goto err;
        }
    } else if (secret == NULL && pkey != NULL) {
        /* TODO combine this with large parts of CRMF_poposigningkey_init() */
        /* EVP_DigestSignInit() checks that pkey type is correct for the alg */

        if (!OBJ_find_sigid_algs(OBJ_obj2nid(algorOID), &md_NID, NULL)
            || (md = EVP_get_digestbynid(md_NID)) == NULL) {
            CMPerr(CMP_F_CMP_CALC_PROTECTION, CMP_R_UNKNOWN_ALGORITHM_ID);
            goto end;
        }
        if ((evp_ctx = EVP_MD_CTX_create()) == NULL
                || EVP_DigestSignInit(evp_ctx, NULL, md, NULL, pkey) <= 0
                || EVP_DigestSignUpdate(evp_ctx, prot_part_der,
                                        prot_part_der_len) <= 0
                || EVP_DigestSignFinal(evp_ctx, NULL, &sig_len) <= 0
                || (protection = OPENSSL_malloc(sig_len)) == NULL
                || EVP_DigestSignFinal(evp_ctx, protection, &sig_len) <= 0)
                goto err;
    } else {
        CMPerr(CMP_F_CMP_CALC_PROTECTION, CMP_R_INVALID_ARGS);
        goto end;
    }

    if ((prot = ASN1_BIT_STRING_new()) == NULL)
        goto err;
    /* OpenSSL defaults all bit strings to be encoded as ASN.1 NamedBitList */
    prot->flags &= ~(ASN1_STRING_FLAG_BITS_LEFT | 0x07);
    prot->flags |= ASN1_STRING_FLAG_BITS_LEFT;
    ASN1_BIT_STRING_set(prot, protection, sig_len);

 err:
    if (prot == NULL)
        CMPerr(CMP_F_CMP_CALC_PROTECTION, CMP_R_ERROR_CALCULATING_PROTECTION);
 end:
    /* cleanup */
    OSSL_CRMF_PBMPARAMETER_free(pbm);
    EVP_MD_CTX_destroy(evp_ctx);
    OPENSSL_free(protection);
    OPENSSL_free(prot_part_der);
    return prot;
}

int OSSL_CMP_MSG_add_extraCerts(OSSL_CMP_CTX *ctx, OSSL_CMP_MSG *msg)
{
    int res = 0;

    if (ctx == NULL || msg == NULL)
        goto err;
    if (msg->extraCerts == NULL
            && (msg->extraCerts = sk_X509_new_null()) == NULL)
        goto err;

    res = 1;
    if (ctx->clCert != NULL) {
        /* Make sure that our own cert gets sent, in the first position */
        res = sk_X509_push(msg->extraCerts, ctx->clCert)
                  && X509_up_ref(ctx->clCert);

        /* if we have untrusted store, try to add intermediate certs */
        if (res != 0 && ctx->untrusted_certs != NULL) {
            STACK_OF(X509) *chain =
                OSSL_CMP_build_cert_chain(ctx->untrusted_certs, ctx->clCert);
            res = OSSL_CMP_sk_X509_add1_certs(msg->extraCerts, chain,
                                              1 /* no self-signed */,
                                              1 /* no duplicates */);
            sk_X509_pop_free(chain, X509_free);
        }
    }

    /* add any additional certificates from ctx->extraCertsOut */
    OSSL_CMP_sk_X509_add1_certs(msg->extraCerts, ctx->extraCertsOut, 0,
                                1 /* no duplicates */);

    /* if none was found avoid empty ASN.1 sequence */
    if (sk_X509_num(msg->extraCerts) == 0) {
        sk_X509_free(msg->extraCerts);
        msg->extraCerts = NULL;
    }
 err:
    return res;
}

/*
 * Create an X509_ALGOR structure for PasswordBasedMAC protection based on
 * the pbm settings in the context
 * returns pointer to X509_ALGOR on success, NULL on error
 */
static X509_ALGOR *CMP_create_pbmac_algor(OSSL_CMP_CTX *ctx)
{
    X509_ALGOR *alg = NULL;
    OSSL_CRMF_PBMPARAMETER *pbm = NULL;
    unsigned char *pbm_der = NULL;
    int pbm_der_len;
    ASN1_STRING *pbm_str = NULL;

    if ((alg = X509_ALGOR_new()) == NULL)
        goto err;
    if ((pbm = OSSL_CRMF_pbmp_new(ctx->pbm_slen, ctx->pbm_owf,
                                  ctx->pbm_itercnt, ctx->pbm_mac)) == NULL)
        goto err;
    if ((pbm_str = ASN1_STRING_new()) == NULL)
        goto err;

    pbm_der_len = i2d_OSSL_CRMF_PBMPARAMETER(pbm, &pbm_der);

    ASN1_STRING_set(pbm_str, pbm_der, pbm_der_len);
    OPENSSL_free(pbm_der);

    X509_ALGOR_set0(alg, OBJ_nid2obj(NID_id_PasswordBasedMAC),
                    V_ASN1_SEQUENCE, pbm_str);

    OSSL_CRMF_PBMPARAMETER_free(pbm);
    return alg;
 err:
    X509_ALGOR_free(alg);
    OSSL_CRMF_PBMPARAMETER_free(pbm);
    return NULL;
}

int OSSL_CMP_MSG_protect(OSSL_CMP_CTX *ctx, OSSL_CMP_MSG *msg)
{
    if (ctx == NULL)
        goto err;
    if (msg == NULL)
        goto err;
    if (ctx->unprotectedSend)
        return 1;

    /* use PasswordBasedMac according to 5.1.3.1 if secretValue is given */
    if (ctx->secretValue != NULL) {
        if ((msg->header->protectionAlg = CMP_create_pbmac_algor(ctx)) == NULL)
            goto err;
        if (ctx->referenceValue != NULL
              && !OSSL_CMP_HDR_set1_senderKID(msg->header, ctx->referenceValue))
            goto err;

        /*
         * add any additional certificates from ctx->extraCertsOut
         * while not needed to validate the signing cert, the option to do
         * this might be handy for certain use cases
         */
        OSSL_CMP_MSG_add_extraCerts(ctx, msg);

        if ((msg->protection =
             CMP_calc_protection(msg, ctx->secretValue, NULL)) == NULL)

            goto err;
    } else {
        /*
         * use MSG_SIG_ALG according to 5.1.3.3 if client Certificate and
         * private key is given
         */
        if (ctx->clCert != NULL && ctx->pkey != NULL) {
            const ASN1_OCTET_STRING *subjKeyIDStr = NULL;
            int algNID = 0;
            ASN1_OBJECT *alg = NULL;

            /* make sure that key and certificate match */
            if (!X509_check_private_key(ctx->clCert, ctx->pkey)) {
                CMPerr(CMP_F_OSSL_CMP_MSG_PROTECT,
                       CMP_R_CERT_AND_KEY_DO_NOT_MATCH);
                goto err;
            }

            if (msg->header->protectionAlg == NULL)
                msg->header->protectionAlg = X509_ALGOR_new();

            if (!OBJ_find_sigid_by_algs(&algNID, ctx->digest,
                        EVP_PKEY_id(ctx->pkey))) {
                CMPerr(CMP_F_OSSL_CMP_MSG_PROTECT,
                       CMP_R_UNSUPPORTED_KEY_TYPE);
                goto err;
            }
            alg = OBJ_nid2obj(algNID);
            X509_ALGOR_set0(msg->header->protectionAlg, alg, V_ASN1_UNDEF,NULL);

            /*
             * set senderKID to  keyIdentifier of the used certificate according
             * to section 5.1.1
             */
            subjKeyIDStr = X509_get0_subject_key_id(ctx->clCert);
            if (subjKeyIDStr != NULL
                    && !OSSL_CMP_HDR_set1_senderKID(msg->header, subjKeyIDStr))
                goto err;

            /*
             * Add ctx->clCert followed, if possible, by its chain built
             * from ctx->untrusted_certs, and then ctx->extraCertsOut
             */
            OSSL_CMP_MSG_add_extraCerts(ctx, msg);

            if ((msg->protection =
                 CMP_calc_protection(msg, NULL, ctx->pkey)) == NULL)
                goto err;
        } else {
            CMPerr(CMP_F_OSSL_CMP_MSG_PROTECT,
                   CMP_R_MISSING_KEY_INPUT_FOR_CREATING_PROTECTION);
            goto err;
        }
    }

    return 1;
 err:
    CMPerr(CMP_F_OSSL_CMP_MSG_PROTECT, CMP_R_ERROR_PROTECTING_MESSAGE);
    return 0;
}

