/*-
 * WARNING: do not edit!
 * Generated by Makefile from include/openssl/crmf.h.in
 *
 * Copyright 2007-2020 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2019
 * Copyright Siemens AG 2015-2019
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * CRMF (RFC 4211) implementation by M. Peylo, M. Viljanen, and D. von Oheimb.
 */



#ifndef OPENSSL_CRMF_H
# define OPENSSL_CRMF_H

# include <openssl/opensslconf.h>

# ifndef OPENSSL_NO_CRMF
#  include <openssl/opensslv.h>
#  include <openssl/safestack.h>
#  include <openssl/crmferr.h>
#  include <openssl/x509v3.h> /* for GENERAL_NAME etc. */

/* explicit #includes not strictly needed since implied by the above: */
#  include <openssl/types.h>
#  include <openssl/x509.h>

#  ifdef __cplusplus
extern "C" {
#  endif

#  define OSSL_CRMF_POPOPRIVKEY_THISMESSAGE          0
#  define OSSL_CRMF_POPOPRIVKEY_SUBSEQUENTMESSAGE    1
#  define OSSL_CRMF_POPOPRIVKEY_DHMAC                2
#  define OSSL_CRMF_POPOPRIVKEY_AGREEMAC             3
#  define OSSL_CRMF_POPOPRIVKEY_ENCRYPTEDKEY         4

#  define OSSL_CRMF_SUBSEQUENTMESSAGE_ENCRCERT       0
#  define OSSL_CRMF_SUBSEQUENTMESSAGE_CHALLENGERESP  1

typedef struct ossl_crmf_encryptedvalue_st OSSL_CRMF_ENCRYPTEDVALUE;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_ENCRYPTEDVALUE)
typedef struct ossl_crmf_encryptedkey_st OSSL_CRMF_ENCRYPTEDKEY;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_ENCRYPTEDKEY)
typedef struct ossl_crmf_msg_st OSSL_CRMF_MSG;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_MSG)
DECLARE_ASN1_DUP_FUNCTION(OSSL_CRMF_MSG)
SKM_DEFINE_STACK_OF_INTERNAL(OSSL_CRMF_MSG, OSSL_CRMF_MSG, OSSL_CRMF_MSG)
#define sk_OSSL_CRMF_MSG_num(sk) OPENSSL_sk_num(ossl_check_const_OSSL_CRMF_MSG_sk_type(sk))
#define sk_OSSL_CRMF_MSG_value(sk, idx) ((OSSL_CRMF_MSG *)OPENSSL_sk_value(ossl_check_const_OSSL_CRMF_MSG_sk_type(sk), (idx)))
#define sk_OSSL_CRMF_MSG_new(cmp) ((STACK_OF(OSSL_CRMF_MSG) *)OPENSSL_sk_new(ossl_check_OSSL_CRMF_MSG_compfunc_type(cmp)))
#define sk_OSSL_CRMF_MSG_new_null() ((STACK_OF(OSSL_CRMF_MSG) *)OPENSSL_sk_new_null())
#define sk_OSSL_CRMF_MSG_new_reserve(cmp, n) ((STACK_OF(OSSL_CRMF_MSG) *)OPENSSL_sk_new_reserve(ossl_check_OSSL_CRMF_MSG_compfunc_type(cmp), (n)))
#define sk_OSSL_CRMF_MSG_reserve(sk, n) OPENSSL_sk_reserve(ossl_check_OSSL_CRMF_MSG_sk_type(sk), (n))
#define sk_OSSL_CRMF_MSG_free(sk) OPENSSL_sk_free(ossl_check_OSSL_CRMF_MSG_sk_type(sk))
#define sk_OSSL_CRMF_MSG_zero(sk) OPENSSL_sk_zero(ossl_check_OSSL_CRMF_MSG_sk_type(sk))
#define sk_OSSL_CRMF_MSG_delete(sk, i) ((OSSL_CRMF_MSG *)OPENSSL_sk_delete(ossl_check_OSSL_CRMF_MSG_sk_type(sk), (i)))
#define sk_OSSL_CRMF_MSG_delete_ptr(sk, ptr) ((OSSL_CRMF_MSG *)OPENSSL_sk_delete_ptr(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr)))
#define sk_OSSL_CRMF_MSG_push(sk, ptr) OPENSSL_sk_push(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr))
#define sk_OSSL_CRMF_MSG_unshift(sk, ptr) OPENSSL_sk_unshift(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr))
#define sk_OSSL_CRMF_MSG_pop(sk) ((OSSL_CRMF_MSG *)OPENSSL_sk_pop(ossl_check_OSSL_CRMF_MSG_sk_type(sk)))
#define sk_OSSL_CRMF_MSG_shift(sk) ((OSSL_CRMF_MSG *)OPENSSL_sk_shift(ossl_check_OSSL_CRMF_MSG_sk_type(sk)))
#define sk_OSSL_CRMF_MSG_pop_free(sk, freefunc) OPENSSL_sk_pop_free(ossl_check_OSSL_CRMF_MSG_sk_type(sk),ossl_check_OSSL_CRMF_MSG_freefunc_type(freefunc))
#define sk_OSSL_CRMF_MSG_insert(sk, ptr, idx) OPENSSL_sk_insert(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr), (idx))
#define sk_OSSL_CRMF_MSG_set(sk, idx, ptr) ((OSSL_CRMF_MSG *)OPENSSL_sk_set(ossl_check_OSSL_CRMF_MSG_sk_type(sk), (idx), ossl_check_OSSL_CRMF_MSG_type(ptr)))
#define sk_OSSL_CRMF_MSG_find(sk, ptr) OPENSSL_sk_find(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr))
#define sk_OSSL_CRMF_MSG_find_ex(sk, ptr) OPENSSL_sk_find_ex(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr))
#define sk_OSSL_CRMF_MSG_find_all(sk, ptr, pnum) OPENSSL_sk_find_all(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_type(ptr), pnum)
#define sk_OSSL_CRMF_MSG_sort(sk) OPENSSL_sk_sort(ossl_check_OSSL_CRMF_MSG_sk_type(sk))
#define sk_OSSL_CRMF_MSG_is_sorted(sk) OPENSSL_sk_is_sorted(ossl_check_const_OSSL_CRMF_MSG_sk_type(sk))
#define sk_OSSL_CRMF_MSG_dup(sk) ((STACK_OF(OSSL_CRMF_MSG) *)OPENSSL_sk_dup(ossl_check_const_OSSL_CRMF_MSG_sk_type(sk)))
#define sk_OSSL_CRMF_MSG_deep_copy(sk, copyfunc, freefunc) ((STACK_OF(OSSL_CRMF_MSG) *)OPENSSL_sk_deep_copy(ossl_check_const_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_copyfunc_type(copyfunc), ossl_check_OSSL_CRMF_MSG_freefunc_type(freefunc)))
#define sk_OSSL_CRMF_MSG_set_cmp_func(sk, cmp) ((sk_OSSL_CRMF_MSG_compfunc)OPENSSL_sk_set_cmp_func(ossl_check_OSSL_CRMF_MSG_sk_type(sk), ossl_check_OSSL_CRMF_MSG_compfunc_type(cmp)))

typedef struct ossl_crmf_attributetypeandvalue_st OSSL_CRMF_ATTRIBUTETYPEANDVALUE;
typedef struct ossl_crmf_pbmparameter_st OSSL_CRMF_PBMPARAMETER;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_PBMPARAMETER)
typedef struct ossl_crmf_poposigningkey_st OSSL_CRMF_POPOSIGNINGKEY;
typedef struct ossl_crmf_certrequest_st OSSL_CRMF_CERTREQUEST;
typedef struct ossl_crmf_certid_st OSSL_CRMF_CERTID;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_CERTID)
DECLARE_ASN1_DUP_FUNCTION(OSSL_CRMF_CERTID)
SKM_DEFINE_STACK_OF_INTERNAL(OSSL_CRMF_CERTID, OSSL_CRMF_CERTID, OSSL_CRMF_CERTID)
#define sk_OSSL_CRMF_CERTID_num(sk) OPENSSL_sk_num(ossl_check_const_OSSL_CRMF_CERTID_sk_type(sk))
#define sk_OSSL_CRMF_CERTID_value(sk, idx) ((OSSL_CRMF_CERTID *)OPENSSL_sk_value(ossl_check_const_OSSL_CRMF_CERTID_sk_type(sk), (idx)))
#define sk_OSSL_CRMF_CERTID_new(cmp) ((STACK_OF(OSSL_CRMF_CERTID) *)OPENSSL_sk_new(ossl_check_OSSL_CRMF_CERTID_compfunc_type(cmp)))
#define sk_OSSL_CRMF_CERTID_new_null() ((STACK_OF(OSSL_CRMF_CERTID) *)OPENSSL_sk_new_null())
#define sk_OSSL_CRMF_CERTID_new_reserve(cmp, n) ((STACK_OF(OSSL_CRMF_CERTID) *)OPENSSL_sk_new_reserve(ossl_check_OSSL_CRMF_CERTID_compfunc_type(cmp), (n)))
#define sk_OSSL_CRMF_CERTID_reserve(sk, n) OPENSSL_sk_reserve(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), (n))
#define sk_OSSL_CRMF_CERTID_free(sk) OPENSSL_sk_free(ossl_check_OSSL_CRMF_CERTID_sk_type(sk))
#define sk_OSSL_CRMF_CERTID_zero(sk) OPENSSL_sk_zero(ossl_check_OSSL_CRMF_CERTID_sk_type(sk))
#define sk_OSSL_CRMF_CERTID_delete(sk, i) ((OSSL_CRMF_CERTID *)OPENSSL_sk_delete(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), (i)))
#define sk_OSSL_CRMF_CERTID_delete_ptr(sk, ptr) ((OSSL_CRMF_CERTID *)OPENSSL_sk_delete_ptr(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr)))
#define sk_OSSL_CRMF_CERTID_push(sk, ptr) OPENSSL_sk_push(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr))
#define sk_OSSL_CRMF_CERTID_unshift(sk, ptr) OPENSSL_sk_unshift(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr))
#define sk_OSSL_CRMF_CERTID_pop(sk) ((OSSL_CRMF_CERTID *)OPENSSL_sk_pop(ossl_check_OSSL_CRMF_CERTID_sk_type(sk)))
#define sk_OSSL_CRMF_CERTID_shift(sk) ((OSSL_CRMF_CERTID *)OPENSSL_sk_shift(ossl_check_OSSL_CRMF_CERTID_sk_type(sk)))
#define sk_OSSL_CRMF_CERTID_pop_free(sk, freefunc) OPENSSL_sk_pop_free(ossl_check_OSSL_CRMF_CERTID_sk_type(sk),ossl_check_OSSL_CRMF_CERTID_freefunc_type(freefunc))
#define sk_OSSL_CRMF_CERTID_insert(sk, ptr, idx) OPENSSL_sk_insert(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr), (idx))
#define sk_OSSL_CRMF_CERTID_set(sk, idx, ptr) ((OSSL_CRMF_CERTID *)OPENSSL_sk_set(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), (idx), ossl_check_OSSL_CRMF_CERTID_type(ptr)))
#define sk_OSSL_CRMF_CERTID_find(sk, ptr) OPENSSL_sk_find(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr))
#define sk_OSSL_CRMF_CERTID_find_ex(sk, ptr) OPENSSL_sk_find_ex(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr))
#define sk_OSSL_CRMF_CERTID_find_all(sk, ptr, pnum) OPENSSL_sk_find_all(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_type(ptr), pnum)
#define sk_OSSL_CRMF_CERTID_sort(sk) OPENSSL_sk_sort(ossl_check_OSSL_CRMF_CERTID_sk_type(sk))
#define sk_OSSL_CRMF_CERTID_is_sorted(sk) OPENSSL_sk_is_sorted(ossl_check_const_OSSL_CRMF_CERTID_sk_type(sk))
#define sk_OSSL_CRMF_CERTID_dup(sk) ((STACK_OF(OSSL_CRMF_CERTID) *)OPENSSL_sk_dup(ossl_check_const_OSSL_CRMF_CERTID_sk_type(sk)))
#define sk_OSSL_CRMF_CERTID_deep_copy(sk, copyfunc, freefunc) ((STACK_OF(OSSL_CRMF_CERTID) *)OPENSSL_sk_deep_copy(ossl_check_const_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_copyfunc_type(copyfunc), ossl_check_OSSL_CRMF_CERTID_freefunc_type(freefunc)))
#define sk_OSSL_CRMF_CERTID_set_cmp_func(sk, cmp) ((sk_OSSL_CRMF_CERTID_compfunc)OPENSSL_sk_set_cmp_func(ossl_check_OSSL_CRMF_CERTID_sk_type(sk), ossl_check_OSSL_CRMF_CERTID_compfunc_type(cmp)))


typedef struct ossl_crmf_pkipublicationinfo_st OSSL_CRMF_PKIPUBLICATIONINFO;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_PKIPUBLICATIONINFO)
typedef struct ossl_crmf_singlepubinfo_st OSSL_CRMF_SINGLEPUBINFO;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_SINGLEPUBINFO)
typedef struct ossl_crmf_certtemplate_st OSSL_CRMF_CERTTEMPLATE;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_CERTTEMPLATE)
typedef STACK_OF(OSSL_CRMF_MSG) OSSL_CRMF_MSGS;
DECLARE_ASN1_FUNCTIONS(OSSL_CRMF_MSGS)

typedef struct ossl_crmf_optionalvalidity_st OSSL_CRMF_OPTIONALVALIDITY;

/* crmf_pbm.c */
OSSL_CRMF_PBMPARAMETER *OSSL_CRMF_pbmp_new(OSSL_LIB_CTX *libctx, size_t slen,
                                           int owfnid, size_t itercnt,
                                           int macnid);
int OSSL_CRMF_pbm_new(OSSL_LIB_CTX *libctx, const char *propq,
                      const OSSL_CRMF_PBMPARAMETER *pbmp,
                      const unsigned char *msg, size_t msglen,
                      const unsigned char *sec, size_t seclen,
                      unsigned char **mac, size_t *maclen);

/* crmf_lib.c */
int OSSL_CRMF_MSG_set1_regCtrl_regToken(OSSL_CRMF_MSG *msg,
                                        const ASN1_UTF8STRING *tok);
ASN1_UTF8STRING
*OSSL_CRMF_MSG_get0_regCtrl_regToken(const OSSL_CRMF_MSG *msg);
int OSSL_CRMF_MSG_set1_regCtrl_authenticator(OSSL_CRMF_MSG *msg,
                                             const ASN1_UTF8STRING *auth);
ASN1_UTF8STRING
*OSSL_CRMF_MSG_get0_regCtrl_authenticator(const OSSL_CRMF_MSG *msg);
int
OSSL_CRMF_MSG_PKIPublicationInfo_push0_SinglePubInfo(OSSL_CRMF_PKIPUBLICATIONINFO *pi,
                                                     OSSL_CRMF_SINGLEPUBINFO *spi);
#  define OSSL_CRMF_PUB_METHOD_DONTCARE 0
#  define OSSL_CRMF_PUB_METHOD_X500     1
#  define OSSL_CRMF_PUB_METHOD_WEB      2
#  define OSSL_CRMF_PUB_METHOD_LDAP     3
int OSSL_CRMF_MSG_set0_SinglePubInfo(OSSL_CRMF_SINGLEPUBINFO *spi,
                                     int method, GENERAL_NAME *nm);
#  define OSSL_CRMF_PUB_ACTION_DONTPUBLISH   0
#  define OSSL_CRMF_PUB_ACTION_PLEASEPUBLISH 1
int OSSL_CRMF_MSG_set_PKIPublicationInfo_action(OSSL_CRMF_PKIPUBLICATIONINFO *pi,
                                                int action);
int OSSL_CRMF_MSG_set1_regCtrl_pkiPublicationInfo(OSSL_CRMF_MSG *msg,
                                                  const OSSL_CRMF_PKIPUBLICATIONINFO *pi);
OSSL_CRMF_PKIPUBLICATIONINFO
*OSSL_CRMF_MSG_get0_regCtrl_pkiPublicationInfo(const OSSL_CRMF_MSG *msg);
int OSSL_CRMF_MSG_set1_regCtrl_protocolEncrKey(OSSL_CRMF_MSG *msg,
                                               const X509_PUBKEY *pubkey);
X509_PUBKEY
*OSSL_CRMF_MSG_get0_regCtrl_protocolEncrKey(const OSSL_CRMF_MSG *msg);
int OSSL_CRMF_MSG_set1_regCtrl_oldCertID(OSSL_CRMF_MSG *msg,
                                         const OSSL_CRMF_CERTID *cid);
OSSL_CRMF_CERTID
*OSSL_CRMF_MSG_get0_regCtrl_oldCertID(const OSSL_CRMF_MSG *msg);
OSSL_CRMF_CERTID *OSSL_CRMF_CERTID_gen(const X509_NAME *issuer,
                                       const ASN1_INTEGER *serial);

int OSSL_CRMF_MSG_set1_regInfo_utf8Pairs(OSSL_CRMF_MSG *msg,
                                         const ASN1_UTF8STRING *utf8pairs);
ASN1_UTF8STRING
*OSSL_CRMF_MSG_get0_regInfo_utf8Pairs(const OSSL_CRMF_MSG *msg);
int OSSL_CRMF_MSG_set1_regInfo_certReq(OSSL_CRMF_MSG *msg,
                                       const OSSL_CRMF_CERTREQUEST *cr);
OSSL_CRMF_CERTREQUEST
*OSSL_CRMF_MSG_get0_regInfo_certReq(const OSSL_CRMF_MSG *msg);

int OSSL_CRMF_MSG_set0_validity(OSSL_CRMF_MSG *crm,
                                ASN1_TIME *notBefore, ASN1_TIME *notAfter);
int OSSL_CRMF_MSG_set_certReqId(OSSL_CRMF_MSG *crm, int rid);
int OSSL_CRMF_MSG_get_certReqId(const OSSL_CRMF_MSG *crm);
int OSSL_CRMF_MSG_set0_extensions(OSSL_CRMF_MSG *crm, X509_EXTENSIONS *exts);

int OSSL_CRMF_MSG_push0_extension(OSSL_CRMF_MSG *crm, X509_EXTENSION *ext);
#  define OSSL_CRMF_POPO_NONE       -1
#  define OSSL_CRMF_POPO_RAVERIFIED 0
#  define OSSL_CRMF_POPO_SIGNATURE  1
#  define OSSL_CRMF_POPO_KEYENC     2
#  define OSSL_CRMF_POPO_KEYAGREE   3
int OSSL_CRMF_MSG_create_popo(int meth, OSSL_CRMF_MSG *crm,
                              EVP_PKEY *pkey, const EVP_MD *digest,
                              OSSL_LIB_CTX *libctx, const char *propq);
int OSSL_CRMF_MSGS_verify_popo(const OSSL_CRMF_MSGS *reqs,
                               int rid, int acceptRAVerified,
                               OSSL_LIB_CTX *libctx, const char *propq);
OSSL_CRMF_CERTTEMPLATE *OSSL_CRMF_MSG_get0_tmpl(const OSSL_CRMF_MSG *crm);
const ASN1_INTEGER
*OSSL_CRMF_CERTTEMPLATE_get0_serialNumber(const OSSL_CRMF_CERTTEMPLATE *tmpl);
const X509_NAME
*OSSL_CRMF_CERTTEMPLATE_get0_subject(const OSSL_CRMF_CERTTEMPLATE *tmpl);
const X509_NAME
*OSSL_CRMF_CERTTEMPLATE_get0_issuer(const OSSL_CRMF_CERTTEMPLATE *tmpl);
X509_EXTENSIONS
*OSSL_CRMF_CERTTEMPLATE_get0_extensions(const OSSL_CRMF_CERTTEMPLATE *tmpl);
const X509_NAME
*OSSL_CRMF_CERTID_get0_issuer(const OSSL_CRMF_CERTID *cid);
const ASN1_INTEGER
*OSSL_CRMF_CERTID_get0_serialNumber(const OSSL_CRMF_CERTID *cid);
int OSSL_CRMF_CERTTEMPLATE_fill(OSSL_CRMF_CERTTEMPLATE *tmpl,
                                EVP_PKEY *pubkey,
                                const X509_NAME *subject,
                                const X509_NAME *issuer,
                                const ASN1_INTEGER *serial);

EEVP_PKEY
*OSSL_CRMF_ENCRYPTEDKEY_get1_privateKey(OSSL_CRMF_ENCRYPTEDKEY* encrytedKey,
                                        EVP_PKEY *ownPk, X509 *ownCert,
                                        X509_STORE *trustedCerts,
                                        STACK_OF(X509) *untrustedCerts,
                                        ASN1_OCTET_STRING *secret);

X509
*OSSL_CRMF_ENCRYPTEDKEY_get1_encCert(const OSSL_CRMF_ENCRYPTEDKEY *ecert,
                                       OSSL_LIB_CTX *libctx, const char *propq,
                                       EVP_PKEY *pkey);

#  ifdef __cplusplus
}
#  endif
# endif /* !defined(OPENSSL_NO_CRMF) */
#endif /* !defined(OPENSSL_CRMF_H) */
