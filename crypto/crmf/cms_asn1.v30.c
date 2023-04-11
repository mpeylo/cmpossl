/*
 * Copyright 2008-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/asn1t.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/cms.h>
/* cms_local.h: */
#ifndef OSSL_CRYPTO_CMS_LOCAL_H
# define OSSL_CRYPTO_CMS_LOCAL_H

# include <openssl/x509.h>

/*
 * Cryptographic message syntax (CMS) structures: taken from RFC3852
 */

/* Forward references */

typedef struct CMS_IssuerAndSerialNumber_st CMS_IssuerAndSerialNumber;
typedef struct CMS_EncapsulatedContentInfo_st CMS_EncapsulatedContentInfo;
typedef struct CMS_SignerIdentifier_st CMS_SignerIdentifier;
/* typedef struct CMS_SignedData_st CMS_SignedData; */
typedef struct CMS_OtherRevocationInfoFormat_st CMS_OtherRevocationInfoFormat;
typedef struct CMS_OriginatorInfo_st CMS_OriginatorInfo;
typedef struct CMS_EncryptedContentInfo_st CMS_EncryptedContentInfo;
/* typedef struct CMS_EnvelopedData_st CMS_EnvelopedData; */
typedef struct CMS_DigestedData_st CMS_DigestedData;
typedef struct CMS_EncryptedData_st CMS_EncryptedData;
typedef struct CMS_AuthenticatedData_st CMS_AuthenticatedData;
typedef struct CMS_AuthEnvelopedData_st CMS_AuthEnvelopedData;
typedef struct CMS_CompressedData_st CMS_CompressedData;
typedef struct CMS_OtherCertificateFormat_st CMS_OtherCertificateFormat;
typedef struct CMS_KeyTransRecipientInfo_st CMS_KeyTransRecipientInfo;
typedef struct CMS_OriginatorPublicKey_st CMS_OriginatorPublicKey;
typedef struct CMS_OriginatorIdentifierOrKey_st CMS_OriginatorIdentifierOrKey;
typedef struct CMS_KeyAgreeRecipientInfo_st CMS_KeyAgreeRecipientInfo;
typedef struct CMS_RecipientKeyIdentifier_st CMS_RecipientKeyIdentifier;
typedef struct CMS_KeyAgreeRecipientIdentifier_st
    CMS_KeyAgreeRecipientIdentifier;
typedef struct CMS_KEKIdentifier_st CMS_KEKIdentifier;
typedef struct CMS_KEKRecipientInfo_st CMS_KEKRecipientInfo;
typedef struct CMS_PasswordRecipientInfo_st CMS_PasswordRecipientInfo;
typedef struct CMS_OtherRecipientInfo_st CMS_OtherRecipientInfo;
typedef struct CMS_ReceiptsFrom_st CMS_ReceiptsFrom;
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

DEFINE_STACK_OF(CMS_CertificateChoices)

struct CMS_SignedData_st {
    int32_t version;
    STACK_OF(X509_ALGOR) *digestAlgorithms;
    CMS_EncapsulatedContentInfo *encapContentInfo;
    STACK_OF(CMS_CertificateChoices) *certificates;
    STACK_OF(CMS_RevocationInfoChoice) *crls;
    STACK_OF(CMS_SignerInfo) *signerInfos;
};

struct CMS_EncapsulatedContentInfo_st {
    ASN1_OBJECT *eContentType;
    ASN1_OCTET_STRING *eContent;
    /* Set to 1 if incomplete structure only part set up */
    int partial;
};

struct CMS_SignerInfo_st {
    int32_t version;
    CMS_SignerIdentifier *sid;
    X509_ALGOR *digestAlgorithm;
    STACK_OF(X509_ATTRIBUTE) *signedAttrs;
    X509_ALGOR *signatureAlgorithm;
    ASN1_OCTET_STRING *signature;
    STACK_OF(X509_ATTRIBUTE) *unsignedAttrs;
    /* Signing certificate and key */
    X509 *signer;
    EVP_PKEY *pkey;
    /* Digest and public key context for alternative parameters */
    EVP_MD_CTX *mctx;
    EVP_PKEY_CTX *pctx;
    const CMS_CTX *cms_ctx;
};

struct CMS_SignerIdentifier_st {
    int type;
    union {
        CMS_IssuerAndSerialNumber *issuerAndSerialNumber;
        ASN1_OCTET_STRING *subjectKeyIdentifier;
    } d;
};

struct CMS_EnvelopedData_st {
    int32_t version;
    CMS_OriginatorInfo *originatorInfo;
    STACK_OF(CMS_RecipientInfo) *recipientInfos;
    CMS_EncryptedContentInfo *encryptedContentInfo;
    STACK_OF(X509_ATTRIBUTE) *unprotectedAttrs;
};

struct CMS_OriginatorInfo_st {
    STACK_OF(CMS_CertificateChoices) *certificates;
    STACK_OF(CMS_RevocationInfoChoice) *crls;
};

struct CMS_EncryptedContentInfo_st {
    ASN1_OBJECT *contentType;
    X509_ALGOR *contentEncryptionAlgorithm;
    ASN1_OCTET_STRING *encryptedContent;
    /* Content encryption algorithm, key and tag */
    const EVP_CIPHER *cipher;
    unsigned char *key;
    size_t keylen;
    unsigned char *tag;
    size_t taglen;
    /* Set to 1 if we are debugging decrypt and don't fake keys for MMA */
    int debug;
    /* Set to 1 if we have no cert and need extra safety measures for MMA */
    int havenocert;
};

struct CMS_RecipientInfo_st {
    int type;
    union {
        CMS_KeyTransRecipientInfo *ktri;
        CMS_KeyAgreeRecipientInfo *kari;
        CMS_KEKRecipientInfo *kekri;
        CMS_PasswordRecipientInfo *pwri;
        CMS_OtherRecipientInfo *ori;
    } d;
};

typedef CMS_SignerIdentifier CMS_RecipientIdentifier;

struct CMS_KeyTransRecipientInfo_st {
    int32_t version;
    CMS_RecipientIdentifier *rid;
    X509_ALGOR *keyEncryptionAlgorithm;
    ASN1_OCTET_STRING *encryptedKey;
    /* Recipient Key and cert */
    X509 *recip;
    EVP_PKEY *pkey;
    /* Public key context for this operation */
    EVP_PKEY_CTX *pctx;
    const CMS_CTX *cms_ctx;
};

struct CMS_KeyAgreeRecipientInfo_st {
    int32_t version;
    CMS_OriginatorIdentifierOrKey *originator;
    ASN1_OCTET_STRING *ukm;
    X509_ALGOR *keyEncryptionAlgorithm;
    STACK_OF(CMS_RecipientEncryptedKey) *recipientEncryptedKeys;
    /* Public key context associated with current operation */
    EVP_PKEY_CTX *pctx;
    /* Cipher context for CEK wrapping */
    EVP_CIPHER_CTX *ctx;
    const CMS_CTX *cms_ctx;
};

struct CMS_OriginatorIdentifierOrKey_st {
    int type;
    union {
        CMS_IssuerAndSerialNumber *issuerAndSerialNumber;
        ASN1_OCTET_STRING *subjectKeyIdentifier;
        CMS_OriginatorPublicKey *originatorKey;
    } d;
};

struct CMS_OriginatorPublicKey_st {
    X509_ALGOR *algorithm;
    ASN1_BIT_STRING *publicKey;
};

struct CMS_RecipientEncryptedKey_st {
    CMS_KeyAgreeRecipientIdentifier *rid;
    ASN1_OCTET_STRING *encryptedKey;
    /* Public key associated with this recipient */
    EVP_PKEY *pkey;
};

struct CMS_KeyAgreeRecipientIdentifier_st {
    int type;
    union {
        CMS_IssuerAndSerialNumber *issuerAndSerialNumber;
        CMS_RecipientKeyIdentifier *rKeyId;
    } d;
};

struct CMS_RecipientKeyIdentifier_st {
    ASN1_OCTET_STRING *subjectKeyIdentifier;
    ASN1_GENERALIZEDTIME *date;
    CMS_OtherKeyAttribute *other;
};

struct CMS_KEKRecipientInfo_st {
    int32_t version;
    CMS_KEKIdentifier *kekid;
    X509_ALGOR *keyEncryptionAlgorithm;
    ASN1_OCTET_STRING *encryptedKey;
    /* Extra info: symmetric key to use */
    unsigned char *key;
    size_t keylen;
    const CMS_CTX *cms_ctx;
};

struct CMS_KEKIdentifier_st {
    ASN1_OCTET_STRING *keyIdentifier;
    ASN1_GENERALIZEDTIME *date;
    CMS_OtherKeyAttribute *other;
};

struct CMS_PasswordRecipientInfo_st {
    int32_t version;
    X509_ALGOR *keyDerivationAlgorithm;
    X509_ALGOR *keyEncryptionAlgorithm;
    ASN1_OCTET_STRING *encryptedKey;
    /* Extra info: password to use */
    unsigned char *pass;
    size_t passlen;
    const CMS_CTX *cms_ctx;
};

struct CMS_OtherRecipientInfo_st {
    ASN1_OBJECT *oriType;
    ASN1_TYPE *oriValue;
};

struct CMS_DigestedData_st {
    int32_t version;
    X509_ALGOR *digestAlgorithm;
    CMS_EncapsulatedContentInfo *encapContentInfo;
    ASN1_OCTET_STRING *digest;
};

struct CMS_EncryptedData_st {
    int32_t version;
    CMS_EncryptedContentInfo *encryptedContentInfo;
    STACK_OF(X509_ATTRIBUTE) *unprotectedAttrs;
};

struct CMS_AuthenticatedData_st {
    int32_t version;
    CMS_OriginatorInfo *originatorInfo;
    STACK_OF(CMS_RecipientInfo) *recipientInfos;
    X509_ALGOR *macAlgorithm;
    X509_ALGOR *digestAlgorithm;
    CMS_EncapsulatedContentInfo *encapContentInfo;
    STACK_OF(X509_ATTRIBUTE) *authAttrs;
    ASN1_OCTET_STRING *mac;
    STACK_OF(X509_ATTRIBUTE) *unauthAttrs;
};

struct CMS_AuthEnvelopedData_st {
    int32_t version;
    CMS_OriginatorInfo *originatorInfo;
    STACK_OF(CMS_RecipientInfo) *recipientInfos;
    CMS_EncryptedContentInfo *authEncryptedContentInfo;
    STACK_OF(X509_ATTRIBUTE) *authAttrs;
    ASN1_OCTET_STRING *mac;
    STACK_OF(X509_ATTRIBUTE) *unauthAttrs;
};

struct CMS_CompressedData_st {
    int32_t version;
    X509_ALGOR *compressionAlgorithm;
    STACK_OF(CMS_RecipientInfo) *recipientInfos;
    CMS_EncapsulatedContentInfo *encapContentInfo;
};

struct CMS_RevocationInfoChoice_st {
    int type;
    union {
        X509_CRL *crl;
        CMS_OtherRevocationInfoFormat *other;
    } d;
};

# define CMS_REVCHOICE_CRL               0
# define CMS_REVCHOICE_OTHER             1

struct CMS_OtherRevocationInfoFormat_st {
    ASN1_OBJECT *otherRevInfoFormat;
    ASN1_TYPE *otherRevInfo;
};

struct CMS_CertificateChoices {
    int type;
    union {
        X509 *certificate;
        ASN1_STRING *extendedCertificate; /* Obsolete */
        ASN1_STRING *v1AttrCert; /* Left encoded for now */
        ASN1_STRING *v2AttrCert; /* Left encoded for now */
        CMS_OtherCertificateFormat *other;
    } d;
};

# define CMS_CERTCHOICE_CERT             0
# define CMS_CERTCHOICE_EXCERT           1
# define CMS_CERTCHOICE_V1ACERT          2
# define CMS_CERTCHOICE_V2ACERT          3
# define CMS_CERTCHOICE_OTHER            4

struct CMS_OtherCertificateFormat_st {
    ASN1_OBJECT *otherCertFormat;
    ASN1_TYPE *otherCert;
};

/*
 * This is also defined in pkcs7.h but we duplicate it to allow the CMS code
 * to be independent of PKCS#7
 */

struct CMS_IssuerAndSerialNumber_st {
    X509_NAME *issuer;
    ASN1_INTEGER *serialNumber;
};

struct CMS_OtherKeyAttribute_st {
    ASN1_OBJECT *keyAttrId;
    ASN1_TYPE *keyAttr;
};

/* ESS structures */

struct CMS_ReceiptRequest_st {
    ASN1_OCTET_STRING *signedContentIdentifier;
    CMS_ReceiptsFrom *receiptsFrom;
    STACK_OF(GENERAL_NAMES) *receiptsTo;
};

struct CMS_ReceiptsFrom_st {
    int type;
    union {
        int32_t allOrFirstTier;
        STACK_OF(GENERAL_NAMES) *receiptList;
    } d;
};

struct CMS_Receipt_st {
    int32_t version;
    ASN1_OBJECT *contentType;
    ASN1_OCTET_STRING *signedContentIdentifier;
    ASN1_OCTET_STRING *originatorSignatureValue;
};

DECLARE_ASN1_FUNCTIONS(CMS_ContentInfo)
DECLARE_ASN1_ITEM(CMS_SignerInfo)
DECLARE_ASN1_ITEM(CMS_IssuerAndSerialNumber)
DECLARE_ASN1_ITEM(CMS_Attributes_Sign)
DECLARE_ASN1_ITEM(CMS_Attributes_Verify)
DECLARE_ASN1_ITEM(CMS_RecipientInfo)
DECLARE_ASN1_ITEM(CMS_PasswordRecipientInfo)
DECLARE_ASN1_ALLOC_FUNCTIONS(CMS_IssuerAndSerialNumber)

# define CMS_SIGNERINFO_ISSUER_SERIAL    0
# define CMS_SIGNERINFO_KEYIDENTIFIER    1

# define CMS_RECIPINFO_ISSUER_SERIAL     0
# define CMS_RECIPINFO_KEYIDENTIFIER     1

# define CMS_REK_ISSUER_SERIAL           0
# define CMS_REK_KEYIDENTIFIER           1

# define CMS_OIK_ISSUER_SERIAL           0
# define CMS_OIK_KEYIDENTIFIER           1
# define CMS_OIK_PUBKEY                  2

BIO *ossl_cms_content_bio(CMS_ContentInfo *cms);
const CMS_CTX *ossl_cms_get0_cmsctx(const CMS_ContentInfo *cms);
OSSL_LIB_CTX *ossl_cms_ctx_get0_libctx(const CMS_CTX *ctx);
const char *ossl_cms_ctx_get0_propq(const CMS_CTX *ctx);
void ossl_cms_resolve_libctx(CMS_ContentInfo *ci);

CMS_ContentInfo *ossl_cms_Data_create(OSSL_LIB_CTX *ctx, const char *propq);

CMS_ContentInfo *ossl_cms_DigestedData_create(const EVP_MD *md,
                                              OSSL_LIB_CTX *libctx,
                                              const char *propq);
BIO *ossl_cms_DigestedData_init_bio(const CMS_ContentInfo *cms);
int ossl_cms_DigestedData_do_final(const CMS_ContentInfo *cms,
                                   BIO *chain, int verify);

BIO *ossl_cms_SignedData_init_bio(CMS_ContentInfo *cms);
int ossl_cms_SignedData_final(CMS_ContentInfo *cms, BIO *chain);
int ossl_cms_set1_SignerIdentifier(CMS_SignerIdentifier *sid, X509 *cert,
                                   int type, const CMS_CTX *ctx);
int ossl_cms_SignerIdentifier_get0_signer_id(CMS_SignerIdentifier *sid,
                                             ASN1_OCTET_STRING **keyid,
                                             X509_NAME **issuer,
                                             ASN1_INTEGER **sno);
int ossl_cms_SignerIdentifier_cert_cmp(CMS_SignerIdentifier *sid, X509 *cert);

CMS_ContentInfo *ossl_cms_CompressedData_create(int comp_nid,
                                                OSSL_LIB_CTX *libctx,
                                                const char *propq);
BIO *ossl_cms_CompressedData_init_bio(const CMS_ContentInfo *cms);

BIO *ossl_cms_DigestAlgorithm_init_bio(X509_ALGOR *digestAlgorithm,
                                       const CMS_CTX *ctx);
int ossl_cms_DigestAlgorithm_find_ctx(EVP_MD_CTX *mctx, BIO *chain,
                                      X509_ALGOR *mdalg);

int ossl_cms_ias_cert_cmp(CMS_IssuerAndSerialNumber *ias, X509 *cert);
int ossl_cms_keyid_cert_cmp(ASN1_OCTET_STRING *keyid, X509 *cert);
int ossl_cms_set1_ias(CMS_IssuerAndSerialNumber **pias, X509 *cert);
int ossl_cms_set1_keyid(ASN1_OCTET_STRING **pkeyid, X509 *cert);

BIO *ossl_cms_EncryptedContent_init_bio(CMS_EncryptedContentInfo *ec,
                                        const CMS_CTX *ctx);
BIO *ossl_cms_EncryptedData_init_bio(const CMS_ContentInfo *cms);
int ossl_cms_EncryptedContent_init(CMS_EncryptedContentInfo *ec,
                                   const EVP_CIPHER *cipher,
                                   const unsigned char *key, size_t keylen,
                                   const CMS_CTX *ctx);

int ossl_cms_Receipt_verify(CMS_ContentInfo *cms, CMS_ContentInfo *req_cms);
int ossl_cms_msgSigDigest_add1(CMS_SignerInfo *dest, CMS_SignerInfo *src);
ASN1_OCTET_STRING *ossl_cms_encode_Receipt(CMS_SignerInfo *si);

BIO *ossl_cms_EnvelopedData_init_bio(CMS_ContentInfo *cms);
int ossl_cms_EnvelopedData_final(CMS_ContentInfo *cms, BIO *chain);
BIO *ossl_cms_AuthEnvelopedData_init_bio(CMS_ContentInfo *cms);
int ossl_cms_AuthEnvelopedData_final(CMS_ContentInfo *cms, BIO *cmsbio);
CMS_EnvelopedData *ossl_cms_get0_enveloped(CMS_ContentInfo *cms);
CMS_AuthEnvelopedData *ossl_cms_get0_auth_enveloped(CMS_ContentInfo *cms);
CMS_EncryptedContentInfo *ossl_cms_get0_env_enc_content(const CMS_ContentInfo *cms);

/* RecipientInfo routines */
int ossl_cms_env_asn1_ctrl(CMS_RecipientInfo *ri, int cmd);
int ossl_cms_pkey_get_ri_type(EVP_PKEY *pk);
int ossl_cms_pkey_is_ri_type_supported(EVP_PKEY *pk, int ri_type);

void ossl_cms_RecipientInfos_set_cmsctx(CMS_ContentInfo *cms);

/* KARI routines */
int ossl_cms_RecipientInfo_kari_init(CMS_RecipientInfo *ri, X509 *recip,
                                     EVP_PKEY *recipPubKey, X509 *originator,
                                     EVP_PKEY *originatorPrivKey,
                                     unsigned int flags,
                                     const CMS_CTX *ctx);
int ossl_cms_RecipientInfo_kari_encrypt(const CMS_ContentInfo *cms,
                                        CMS_RecipientInfo *ri);

/* PWRI routines */
int ossl_cms_RecipientInfo_pwri_crypt(const CMS_ContentInfo *cms,
                                      CMS_RecipientInfo *ri, int en_de);
/* SignerInfo routines */
int ossl_cms_si_check_attributes(const CMS_SignerInfo *si);
void ossl_cms_SignerInfos_set_cmsctx(CMS_ContentInfo *cms);


/* ESS routines */
int ossl_cms_check_signing_certs(const CMS_SignerInfo *si,
                                 const STACK_OF(X509) *chain);

int ossl_cms_dh_envelope(CMS_RecipientInfo *ri, int decrypt);
int ossl_cms_ecdh_envelope(CMS_RecipientInfo *ri, int decrypt);
int ossl_cms_rsa_envelope(CMS_RecipientInfo *ri, int decrypt);
int ossl_cms_ecdsa_dsa_sign(CMS_SignerInfo *si, int verify);
int ossl_cms_rsa_sign(CMS_SignerInfo *si, int verify);

DECLARE_ASN1_ITEM(CMS_CertificateChoices)
DECLARE_ASN1_ITEM(CMS_DigestedData)
DECLARE_ASN1_ITEM(CMS_EncryptedData)
DECLARE_ASN1_ITEM(CMS_EnvelopedData)
DECLARE_ASN1_ITEM(CMS_AuthEnvelopedData)
DECLARE_ASN1_ITEM(CMS_KEKRecipientInfo)
DECLARE_ASN1_ITEM(CMS_KeyAgreeRecipientInfo)
DECLARE_ASN1_ITEM(CMS_KeyTransRecipientInfo)
DECLARE_ASN1_ITEM(CMS_OriginatorPublicKey)
DECLARE_ASN1_ITEM(CMS_OtherKeyAttribute)
DECLARE_ASN1_ITEM(CMS_Receipt)
DECLARE_ASN1_ITEM(CMS_ReceiptRequest)
DECLARE_ASN1_ITEM(CMS_RecipientEncryptedKey)
DECLARE_ASN1_ITEM(CMS_RecipientKeyIdentifier)
DECLARE_ASN1_ITEM(CMS_RevocationInfoChoice)
DECLARE_ASN1_ITEM(CMS_SignedData)
DECLARE_ASN1_ITEM(CMS_CompressedData)

#endif /* ndef OSSL_CRYPTO_CMS_LOCAL_H */


ASN1_SEQUENCE(CMS_IssuerAndSerialNumber) = {
        ASN1_SIMPLE(CMS_IssuerAndSerialNumber, issuer, X509_NAME),
        ASN1_SIMPLE(CMS_IssuerAndSerialNumber, serialNumber, ASN1_INTEGER)
} ASN1_SEQUENCE_END(CMS_IssuerAndSerialNumber)

ASN1_SEQUENCE(CMS_OtherCertificateFormat) = {
        ASN1_SIMPLE(CMS_OtherCertificateFormat, otherCertFormat, ASN1_OBJECT),
        ASN1_OPT(CMS_OtherCertificateFormat, otherCert, ASN1_ANY)
} static_ASN1_SEQUENCE_END(CMS_OtherCertificateFormat)

ASN1_CHOICE(CMS_CertificateChoices) = {
        ASN1_SIMPLE(CMS_CertificateChoices, d.certificate, X509),
        ASN1_IMP(CMS_CertificateChoices, d.extendedCertificate, ASN1_SEQUENCE, 0),
        ASN1_IMP(CMS_CertificateChoices, d.v1AttrCert, ASN1_SEQUENCE, 1),
        ASN1_IMP(CMS_CertificateChoices, d.v2AttrCert, ASN1_SEQUENCE, 2),
        ASN1_IMP(CMS_CertificateChoices, d.other, CMS_OtherCertificateFormat, 3)
} ASN1_CHOICE_END(CMS_CertificateChoices)

ASN1_CHOICE(CMS_SignerIdentifier) = {
        ASN1_SIMPLE(CMS_SignerIdentifier, d.issuerAndSerialNumber, CMS_IssuerAndSerialNumber),
        ASN1_IMP(CMS_SignerIdentifier, d.subjectKeyIdentifier, ASN1_OCTET_STRING, 0)
} static_ASN1_CHOICE_END(CMS_SignerIdentifier)

ASN1_NDEF_SEQUENCE(CMS_EncapsulatedContentInfo) = {
        ASN1_SIMPLE(CMS_EncapsulatedContentInfo, eContentType, ASN1_OBJECT),
        ASN1_NDEF_EXP_OPT(CMS_EncapsulatedContentInfo, eContent, ASN1_OCTET_STRING_NDEF, 0)
} static_ASN1_NDEF_SEQUENCE_END(CMS_EncapsulatedContentInfo)

/* Minor tweak to operation: free up signer key, cert */
static int cms_si_cb(int operation, ASN1_VALUE **pval,
                     ossl_unused const ASN1_ITEM *it, ossl_unused void *exarg)
{
    if (operation == ASN1_OP_FREE_POST) {
        CMS_SignerInfo *si = (CMS_SignerInfo *)*pval;
        EVP_PKEY_free(si->pkey);
        X509_free(si->signer);
        EVP_MD_CTX_free(si->mctx);
    }
    return 1;
}

ASN1_SEQUENCE_cb(CMS_SignerInfo, cms_si_cb) = {
        ASN1_EMBED(CMS_SignerInfo, version, INT32),
        ASN1_SIMPLE(CMS_SignerInfo, sid, CMS_SignerIdentifier),
        ASN1_SIMPLE(CMS_SignerInfo, digestAlgorithm, X509_ALGOR),
        ASN1_IMP_SET_OF_OPT(CMS_SignerInfo, signedAttrs, X509_ATTRIBUTE, 0),
        ASN1_SIMPLE(CMS_SignerInfo, signatureAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_SignerInfo, signature, ASN1_OCTET_STRING),
        ASN1_IMP_SET_OF_OPT(CMS_SignerInfo, unsignedAttrs, X509_ATTRIBUTE, 1)
} ASN1_SEQUENCE_END_cb(CMS_SignerInfo, CMS_SignerInfo)

ASN1_SEQUENCE(CMS_OtherRevocationInfoFormat) = {
        ASN1_SIMPLE(CMS_OtherRevocationInfoFormat, otherRevInfoFormat, ASN1_OBJECT),
        ASN1_OPT(CMS_OtherRevocationInfoFormat, otherRevInfo, ASN1_ANY)
} static_ASN1_SEQUENCE_END(CMS_OtherRevocationInfoFormat)

ASN1_CHOICE(CMS_RevocationInfoChoice) = {
        ASN1_SIMPLE(CMS_RevocationInfoChoice, d.crl, X509_CRL),
        ASN1_IMP(CMS_RevocationInfoChoice, d.other, CMS_OtherRevocationInfoFormat, 1)
} ASN1_CHOICE_END(CMS_RevocationInfoChoice)

ASN1_NDEF_SEQUENCE(CMS_SignedData) = {
        ASN1_EMBED(CMS_SignedData, version, INT32),
        ASN1_SET_OF(CMS_SignedData, digestAlgorithms, X509_ALGOR),
        ASN1_SIMPLE(CMS_SignedData, encapContentInfo, CMS_EncapsulatedContentInfo),
        ASN1_IMP_SET_OF_OPT(CMS_SignedData, certificates, CMS_CertificateChoices, 0),
        ASN1_IMP_SET_OF_OPT(CMS_SignedData, crls, CMS_RevocationInfoChoice, 1),
        ASN1_SET_OF(CMS_SignedData, signerInfos, CMS_SignerInfo)
} ASN1_NDEF_SEQUENCE_END(CMS_SignedData)

ASN1_SEQUENCE(CMS_OriginatorInfo) = {
        ASN1_IMP_SET_OF_OPT(CMS_OriginatorInfo, certificates, CMS_CertificateChoices, 0),
        ASN1_IMP_SET_OF_OPT(CMS_OriginatorInfo, crls, CMS_RevocationInfoChoice, 1)
} static_ASN1_SEQUENCE_END(CMS_OriginatorInfo)

ASN1_NDEF_SEQUENCE(CMS_EncryptedContentInfo) = {
        ASN1_SIMPLE(CMS_EncryptedContentInfo, contentType, ASN1_OBJECT),
        ASN1_SIMPLE(CMS_EncryptedContentInfo, contentEncryptionAlgorithm, X509_ALGOR),
        ASN1_IMP_OPT(CMS_EncryptedContentInfo, encryptedContent, ASN1_OCTET_STRING_NDEF, 0)
} static_ASN1_NDEF_SEQUENCE_END(CMS_EncryptedContentInfo)

ASN1_SEQUENCE(CMS_KeyTransRecipientInfo) = {
        ASN1_EMBED(CMS_KeyTransRecipientInfo, version, INT32),
        ASN1_SIMPLE(CMS_KeyTransRecipientInfo, rid, CMS_SignerIdentifier),
        ASN1_SIMPLE(CMS_KeyTransRecipientInfo, keyEncryptionAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_KeyTransRecipientInfo, encryptedKey, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CMS_KeyTransRecipientInfo)

ASN1_SEQUENCE(CMS_OtherKeyAttribute) = {
        ASN1_SIMPLE(CMS_OtherKeyAttribute, keyAttrId, ASN1_OBJECT),
        ASN1_OPT(CMS_OtherKeyAttribute, keyAttr, ASN1_ANY)
} ASN1_SEQUENCE_END(CMS_OtherKeyAttribute)

ASN1_SEQUENCE(CMS_RecipientKeyIdentifier) = {
        ASN1_SIMPLE(CMS_RecipientKeyIdentifier, subjectKeyIdentifier, ASN1_OCTET_STRING),
        ASN1_OPT(CMS_RecipientKeyIdentifier, date, ASN1_GENERALIZEDTIME),
        ASN1_OPT(CMS_RecipientKeyIdentifier, other, CMS_OtherKeyAttribute)
} ASN1_SEQUENCE_END(CMS_RecipientKeyIdentifier)

ASN1_CHOICE(CMS_KeyAgreeRecipientIdentifier) = {
  ASN1_SIMPLE(CMS_KeyAgreeRecipientIdentifier, d.issuerAndSerialNumber, CMS_IssuerAndSerialNumber),
  ASN1_IMP(CMS_KeyAgreeRecipientIdentifier, d.rKeyId, CMS_RecipientKeyIdentifier, 0)
} static_ASN1_CHOICE_END(CMS_KeyAgreeRecipientIdentifier)

static int cms_rek_cb(int operation, ASN1_VALUE **pval,
                      ossl_unused const ASN1_ITEM *it, ossl_unused void *exarg)
{
    CMS_RecipientEncryptedKey *rek = (CMS_RecipientEncryptedKey *)*pval;
    if (operation == ASN1_OP_FREE_POST) {
        EVP_PKEY_free(rek->pkey);
    }
    return 1;
}

ASN1_SEQUENCE_cb(CMS_RecipientEncryptedKey, cms_rek_cb) = {
        ASN1_SIMPLE(CMS_RecipientEncryptedKey, rid, CMS_KeyAgreeRecipientIdentifier),
        ASN1_SIMPLE(CMS_RecipientEncryptedKey, encryptedKey, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END_cb(CMS_RecipientEncryptedKey, CMS_RecipientEncryptedKey)

ASN1_SEQUENCE(CMS_OriginatorPublicKey) = {
  ASN1_SIMPLE(CMS_OriginatorPublicKey, algorithm, X509_ALGOR),
  ASN1_SIMPLE(CMS_OriginatorPublicKey, publicKey, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(CMS_OriginatorPublicKey)

ASN1_CHOICE(CMS_OriginatorIdentifierOrKey) = {
  ASN1_SIMPLE(CMS_OriginatorIdentifierOrKey, d.issuerAndSerialNumber, CMS_IssuerAndSerialNumber),
  ASN1_IMP(CMS_OriginatorIdentifierOrKey, d.subjectKeyIdentifier, ASN1_OCTET_STRING, 0),
  ASN1_IMP(CMS_OriginatorIdentifierOrKey, d.originatorKey, CMS_OriginatorPublicKey, 1)
} static_ASN1_CHOICE_END(CMS_OriginatorIdentifierOrKey)

static int cms_kari_cb(int operation, ASN1_VALUE **pval,
                       ossl_unused const ASN1_ITEM *it, ossl_unused void *exarg)
{
    CMS_KeyAgreeRecipientInfo *kari = (CMS_KeyAgreeRecipientInfo *)*pval;
    if (operation == ASN1_OP_NEW_POST) {
        kari->ctx = EVP_CIPHER_CTX_new();
        if (kari->ctx == NULL)
            return 0;
        EVP_CIPHER_CTX_set_flags(kari->ctx, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
        kari->pctx = NULL;
    } else if (operation == ASN1_OP_FREE_POST) {
        EVP_PKEY_CTX_free(kari->pctx);
        EVP_CIPHER_CTX_free(kari->ctx);
    }
    return 1;
}

ASN1_SEQUENCE_cb(CMS_KeyAgreeRecipientInfo, cms_kari_cb) = {
        ASN1_EMBED(CMS_KeyAgreeRecipientInfo, version, INT32),
        ASN1_EXP(CMS_KeyAgreeRecipientInfo, originator, CMS_OriginatorIdentifierOrKey, 0),
        ASN1_EXP_OPT(CMS_KeyAgreeRecipientInfo, ukm, ASN1_OCTET_STRING, 1),
        ASN1_SIMPLE(CMS_KeyAgreeRecipientInfo, keyEncryptionAlgorithm, X509_ALGOR),
        ASN1_SEQUENCE_OF(CMS_KeyAgreeRecipientInfo, recipientEncryptedKeys, CMS_RecipientEncryptedKey)
} ASN1_SEQUENCE_END_cb(CMS_KeyAgreeRecipientInfo, CMS_KeyAgreeRecipientInfo)

ASN1_SEQUENCE(CMS_KEKIdentifier) = {
        ASN1_SIMPLE(CMS_KEKIdentifier, keyIdentifier, ASN1_OCTET_STRING),
        ASN1_OPT(CMS_KEKIdentifier, date, ASN1_GENERALIZEDTIME),
        ASN1_OPT(CMS_KEKIdentifier, other, CMS_OtherKeyAttribute)
} static_ASN1_SEQUENCE_END(CMS_KEKIdentifier)

ASN1_SEQUENCE(CMS_KEKRecipientInfo) = {
        ASN1_EMBED(CMS_KEKRecipientInfo, version, INT32),
        ASN1_SIMPLE(CMS_KEKRecipientInfo, kekid, CMS_KEKIdentifier),
        ASN1_SIMPLE(CMS_KEKRecipientInfo, keyEncryptionAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_KEKRecipientInfo, encryptedKey, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CMS_KEKRecipientInfo)

ASN1_SEQUENCE(CMS_PasswordRecipientInfo) = {
        ASN1_EMBED(CMS_PasswordRecipientInfo, version, INT32),
        ASN1_IMP_OPT(CMS_PasswordRecipientInfo, keyDerivationAlgorithm, X509_ALGOR, 0),
        ASN1_SIMPLE(CMS_PasswordRecipientInfo, keyEncryptionAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_PasswordRecipientInfo, encryptedKey, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CMS_PasswordRecipientInfo)

ASN1_SEQUENCE(CMS_OtherRecipientInfo) = {
  ASN1_SIMPLE(CMS_OtherRecipientInfo, oriType, ASN1_OBJECT),
  ASN1_OPT(CMS_OtherRecipientInfo, oriValue, ASN1_ANY)
} static_ASN1_SEQUENCE_END(CMS_OtherRecipientInfo)

/* Free up RecipientInfo additional data */
static int cms_ri_cb(int operation, ASN1_VALUE **pval,
                     ossl_unused const ASN1_ITEM *it, ossl_unused void *exarg)
{
    if (operation == ASN1_OP_FREE_PRE) {
        CMS_RecipientInfo *ri = (CMS_RecipientInfo *)*pval;
        if (ri->type == CMS_RECIPINFO_TRANS) {
            CMS_KeyTransRecipientInfo *ktri = ri->d.ktri;
            EVP_PKEY_free(ktri->pkey);
            X509_free(ktri->recip);
            EVP_PKEY_CTX_free(ktri->pctx);
        } else if (ri->type == CMS_RECIPINFO_KEK) {
            CMS_KEKRecipientInfo *kekri = ri->d.kekri;
            OPENSSL_clear_free(kekri->key, kekri->keylen);
        } else if (ri->type == CMS_RECIPINFO_PASS) {
            CMS_PasswordRecipientInfo *pwri = ri->d.pwri;
            OPENSSL_clear_free(pwri->pass, pwri->passlen);
        }
    }
    return 1;
}

ASN1_CHOICE_cb(CMS_RecipientInfo, cms_ri_cb) = {
        ASN1_SIMPLE(CMS_RecipientInfo, d.ktri, CMS_KeyTransRecipientInfo),
        ASN1_IMP(CMS_RecipientInfo, d.kari, CMS_KeyAgreeRecipientInfo, 1),
        ASN1_IMP(CMS_RecipientInfo, d.kekri, CMS_KEKRecipientInfo, 2),
        ASN1_IMP(CMS_RecipientInfo, d.pwri, CMS_PasswordRecipientInfo, 3),
        ASN1_IMP(CMS_RecipientInfo, d.ori, CMS_OtherRecipientInfo, 4)
} ASN1_CHOICE_END_cb(CMS_RecipientInfo, CMS_RecipientInfo, type)

ASN1_NDEF_SEQUENCE(CMS_EnvelopedData) = {
        ASN1_EMBED(CMS_EnvelopedData, version, INT32),
        ASN1_IMP_OPT(CMS_EnvelopedData, originatorInfo, CMS_OriginatorInfo, 0),
        ASN1_SET_OF(CMS_EnvelopedData, recipientInfos, CMS_RecipientInfo),
        ASN1_SIMPLE(CMS_EnvelopedData, encryptedContentInfo, CMS_EncryptedContentInfo),
        ASN1_IMP_SET_OF_OPT(CMS_EnvelopedData, unprotectedAttrs, X509_ATTRIBUTE, 1)
} ASN1_NDEF_SEQUENCE_END(CMS_EnvelopedData)

ASN1_NDEF_SEQUENCE(CMS_DigestedData) = {
        ASN1_EMBED(CMS_DigestedData, version, INT32),
        ASN1_SIMPLE(CMS_DigestedData, digestAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_DigestedData, encapContentInfo, CMS_EncapsulatedContentInfo),
        ASN1_SIMPLE(CMS_DigestedData, digest, ASN1_OCTET_STRING)
} ASN1_NDEF_SEQUENCE_END(CMS_DigestedData)

ASN1_NDEF_SEQUENCE(CMS_EncryptedData) = {
        ASN1_EMBED(CMS_EncryptedData, version, INT32),
        ASN1_SIMPLE(CMS_EncryptedData, encryptedContentInfo, CMS_EncryptedContentInfo),
        ASN1_IMP_SET_OF_OPT(CMS_EncryptedData, unprotectedAttrs, X509_ATTRIBUTE, 1)
} ASN1_NDEF_SEQUENCE_END(CMS_EncryptedData)

/* Defined in RFC 5083 - Section 2.1. AuthEnvelopedData Type */
ASN1_NDEF_SEQUENCE(CMS_AuthEnvelopedData) = {
        ASN1_EMBED(CMS_AuthEnvelopedData, version, INT32),
        ASN1_IMP_OPT(CMS_AuthEnvelopedData, originatorInfo, CMS_OriginatorInfo, 0),
        ASN1_SET_OF(CMS_AuthEnvelopedData, recipientInfos, CMS_RecipientInfo),
        ASN1_SIMPLE(CMS_AuthEnvelopedData, authEncryptedContentInfo, CMS_EncryptedContentInfo),
        ASN1_IMP_SET_OF_OPT(CMS_AuthEnvelopedData, authAttrs, X509_ALGOR, 2),
        ASN1_SIMPLE(CMS_AuthEnvelopedData, mac, ASN1_OCTET_STRING),
        ASN1_IMP_SET_OF_OPT(CMS_AuthEnvelopedData, unauthAttrs, X509_ALGOR, 3)
} ASN1_NDEF_SEQUENCE_END(CMS_AuthEnvelopedData)

ASN1_NDEF_SEQUENCE(CMS_AuthenticatedData) = {
        ASN1_EMBED(CMS_AuthenticatedData, version, INT32),
        ASN1_IMP_OPT(CMS_AuthenticatedData, originatorInfo, CMS_OriginatorInfo, 0),
        ASN1_SET_OF(CMS_AuthenticatedData, recipientInfos, CMS_RecipientInfo),
        ASN1_SIMPLE(CMS_AuthenticatedData, macAlgorithm, X509_ALGOR),
        ASN1_IMP(CMS_AuthenticatedData, digestAlgorithm, X509_ALGOR, 1),
        ASN1_SIMPLE(CMS_AuthenticatedData, encapContentInfo, CMS_EncapsulatedContentInfo),
        ASN1_IMP_SET_OF_OPT(CMS_AuthenticatedData, authAttrs, X509_ALGOR, 2),
        ASN1_SIMPLE(CMS_AuthenticatedData, mac, ASN1_OCTET_STRING),
        ASN1_IMP_SET_OF_OPT(CMS_AuthenticatedData, unauthAttrs, X509_ALGOR, 3)
} static_ASN1_NDEF_SEQUENCE_END(CMS_AuthenticatedData)

ASN1_NDEF_SEQUENCE(CMS_CompressedData) = {
        ASN1_EMBED(CMS_CompressedData, version, INT32),
        ASN1_SIMPLE(CMS_CompressedData, compressionAlgorithm, X509_ALGOR),
        ASN1_SIMPLE(CMS_CompressedData, encapContentInfo, CMS_EncapsulatedContentInfo),
} ASN1_NDEF_SEQUENCE_END(CMS_CompressedData)

/* This is the ANY DEFINED BY table for the top level ContentInfo structure */

ASN1_ADB_TEMPLATE(cms_default) = ASN1_EXP(CMS_ContentInfo, d.other, ASN1_ANY, 0);

ASN1_ADB(CMS_ContentInfo) = {
        ADB_ENTRY(NID_pkcs7_data, ASN1_NDEF_EXP(CMS_ContentInfo, d.data, ASN1_OCTET_STRING_NDEF, 0)),
        ADB_ENTRY(NID_pkcs7_signed, ASN1_NDEF_EXP(CMS_ContentInfo, d.signedData, CMS_SignedData, 0)),
        ADB_ENTRY(NID_pkcs7_enveloped, ASN1_NDEF_EXP(CMS_ContentInfo, d.envelopedData, CMS_EnvelopedData, 0)),
        ADB_ENTRY(NID_pkcs7_digest, ASN1_NDEF_EXP(CMS_ContentInfo, d.digestedData, CMS_DigestedData, 0)),
        ADB_ENTRY(NID_pkcs7_encrypted, ASN1_NDEF_EXP(CMS_ContentInfo, d.encryptedData, CMS_EncryptedData, 0)),
        ADB_ENTRY(NID_id_smime_ct_authEnvelopedData, ASN1_NDEF_EXP(CMS_ContentInfo, d.authEnvelopedData, CMS_AuthEnvelopedData, 0)),
        ADB_ENTRY(NID_id_smime_ct_authData, ASN1_NDEF_EXP(CMS_ContentInfo, d.authenticatedData, CMS_AuthenticatedData, 0)),
        ADB_ENTRY(NID_id_smime_ct_compressedData, ASN1_NDEF_EXP(CMS_ContentInfo, d.compressedData, CMS_CompressedData, 0)),
} ASN1_ADB_END(CMS_ContentInfo, 0, contentType, 0, &cms_default_tt, NULL);

/* CMS streaming support */
static int cms_cb(int operation, ASN1_VALUE **pval,
                  ossl_unused const ASN1_ITEM *it, void *exarg)
{
    ASN1_STREAM_ARG *sarg = exarg;
    CMS_ContentInfo *cms = NULL;
    if (pval)
        cms = (CMS_ContentInfo *)*pval;
    else
        return 1;
    switch (operation) {

    case ASN1_OP_STREAM_PRE:
        if (CMS_stream(&sarg->boundary, cms) <= 0)
            return 0;
        /* fall thru */
    case ASN1_OP_DETACHED_PRE:
        sarg->ndef_bio = CMS_dataInit(cms, sarg->out);
        if (!sarg->ndef_bio)
            return 0;
        break;

    case ASN1_OP_STREAM_POST:
    case ASN1_OP_DETACHED_POST:
        if (CMS_dataFinal(cms, sarg->ndef_bio) <= 0)
            return 0;
        break;

    }
    return 1;
}

ASN1_NDEF_SEQUENCE_cb(CMS_ContentInfo, cms_cb) = {
        ASN1_SIMPLE(CMS_ContentInfo, contentType, ASN1_OBJECT),
        ASN1_ADB_OBJECT(CMS_ContentInfo)
} ASN1_NDEF_SEQUENCE_END_cb(CMS_ContentInfo, CMS_ContentInfo)

/* Specials for signed attributes */

/*
 * When signing attributes we want to reorder them to match the sorted
 * encoding.
 */

ASN1_ITEM_TEMPLATE(CMS_Attributes_Sign) =
        ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SET_ORDER, 0, CMS_ATTRIBUTES, X509_ATTRIBUTE)
ASN1_ITEM_TEMPLATE_END(CMS_Attributes_Sign)

/*
 * When verifying attributes we need to use the received order. So we use
 * SEQUENCE OF and tag it to SET OF
 */

ASN1_ITEM_TEMPLATE(CMS_Attributes_Verify) =
        ASN1_EX_TEMPLATE_TYPE(ASN1_TFLG_SEQUENCE_OF | ASN1_TFLG_IMPTAG | ASN1_TFLG_UNIVERSAL,
                                V_ASN1_SET, CMS_ATTRIBUTES, X509_ATTRIBUTE)
ASN1_ITEM_TEMPLATE_END(CMS_Attributes_Verify)



ASN1_CHOICE(CMS_ReceiptsFrom) = {
  ASN1_IMP_EMBED(CMS_ReceiptsFrom, d.allOrFirstTier, INT32, 0),
  ASN1_IMP_SEQUENCE_OF(CMS_ReceiptsFrom, d.receiptList, GENERAL_NAMES, 1)
} static_ASN1_CHOICE_END(CMS_ReceiptsFrom)

ASN1_SEQUENCE(CMS_ReceiptRequest) = {
  ASN1_SIMPLE(CMS_ReceiptRequest, signedContentIdentifier, ASN1_OCTET_STRING),
  ASN1_SIMPLE(CMS_ReceiptRequest, receiptsFrom, CMS_ReceiptsFrom),
  ASN1_SEQUENCE_OF(CMS_ReceiptRequest, receiptsTo, GENERAL_NAMES)
} ASN1_SEQUENCE_END(CMS_ReceiptRequest)

ASN1_SEQUENCE(CMS_Receipt) = {
  ASN1_EMBED(CMS_Receipt, version, INT32),
  ASN1_SIMPLE(CMS_Receipt, contentType, ASN1_OBJECT),
  ASN1_SIMPLE(CMS_Receipt, signedContentIdentifier, ASN1_OCTET_STRING),
  ASN1_SIMPLE(CMS_Receipt, originatorSignatureValue, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CMS_Receipt)

/*
 * Utilities to encode the CMS_SharedInfo structure used during key
 * derivation.
 */

typedef struct {
    X509_ALGOR *keyInfo;
    ASN1_OCTET_STRING *entityUInfo;
    ASN1_OCTET_STRING *suppPubInfo;
} CMS_SharedInfo;

ASN1_SEQUENCE(CMS_SharedInfo) = {
  ASN1_SIMPLE(CMS_SharedInfo, keyInfo, X509_ALGOR),
  ASN1_EXP_OPT(CMS_SharedInfo, entityUInfo, ASN1_OCTET_STRING, 0),
  ASN1_EXP_OPT(CMS_SharedInfo, suppPubInfo, ASN1_OCTET_STRING, 2),
} static_ASN1_SEQUENCE_END(CMS_SharedInfo)

int CMS_SharedInfo_encode(unsigned char **pder, X509_ALGOR *kekalg,
                          ASN1_OCTET_STRING *ukm, int keylen)
{
    union {
        CMS_SharedInfo *pecsi;
        ASN1_VALUE *a;
    } intsi = {
        NULL
    };

    ASN1_OCTET_STRING oklen;
    unsigned char kl[4];
    CMS_SharedInfo ecsi;

    keylen <<= 3;
    kl[0] = (keylen >> 24) & 0xff;
    kl[1] = (keylen >> 16) & 0xff;
    kl[2] = (keylen >> 8) & 0xff;
    kl[3] = keylen & 0xff;
    oklen.length = 4;
    oklen.data = kl;
    oklen.type = V_ASN1_OCTET_STRING;
    oklen.flags = 0;
    ecsi.keyInfo = kekalg;
    ecsi.entityUInfo = ukm;
    ecsi.suppPubInfo = &oklen;
    intsi.pecsi = &ecsi;
    return ASN1_item_i2d(intsi.a, pder, ASN1_ITEM_rptr(CMS_SharedInfo));
}
