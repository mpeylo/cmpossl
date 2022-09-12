/*-
 * Copyright 2007-2020 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright Nokia 2007-2019
 * Copyright Siemens AG 2015-2019
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_BACKPORT_H
# define OPENSSL_BACKPORT_H

# include <stddef.h> /* size_t */

# include <openssl/opensslv.h>
# if OPENSSL_VERSION_NUMBER < 0x30000000L && !defined(CMP_STANDALONE)
#  define CMP_STANDALONE
# endif

# if OPENSSL_VERSION_NUMBER < 0x10100000L
/* compilation quirks for OpenSSL <= 1.0.2 */
_Pragma("GCC diagnostic ignored \"-Wdiscarded-qualifiers\"")
_Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
# endif

# include <openssl/e_os2.h> /* for ossl_inline etc. */
# include <openssl/x509v3.h> /* for GENERAL_NAME, X509_ALGOR, etc. */

# if OPENSSL_VERSION_NUMBER < 0x30100000L
X509_ALGOR *ossl_X509_ALGOR_from_nid(int nid, int ptype, void *pval);
#  define OSSL_STACK_OF_X509_free(sk) sk_X509_pop_free(sk, X509_free)
#  define OPENSSL_strcasecmp  strcasecmp
#  define OPENSSL_strncasecmp strncasecmp
#  define ossl_asn1_string_set_bits_left(str, num) \
    ((str)->flags &= ~0x07, \
     (str)->flags |= ASN1_STRING_FLAG_BITS_LEFT | ((num) & 0x07))
# endif

# if OPENSSL_VERSION_NUMBER < 0x30000000L
#  define ossl_unused __attribute__((unused))
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100002L
#  define __owur
#  define OPENSSL_FILE __FILE__
#  define OPENSSL_LINE __LINE__
# endif

# ifdef CMP_STANDALONE
#  if OPENSSL_VERSION_NUMBER < 0x10101000L
#   define OPENSSL_sk_new_reserve(f, n) sk_new(f) /* sorry, no reservation */
#   define OPENSSL_sk_reserve(sk, n) 1 /* sorry, no-op */
static ossl_unused ossl_inline int \
    ERR_clear_last_mark(void) { return 1; } /* sorry, no-op */
#   define HEADER_ASN1ERR_H /* block inclusion of header added in 1.1.1 */
#  endif
#  if OPENSSL_VERSION_NUMBER < 0x10100006L
#   include <openssl/safestack_backport.h>
#  endif
# endif

# ifdef CMP_STANDALONE
#  if OPENSSL_VERSION_NUMBER < 0x10101000L
#   include <openssl/err.h>
int ERR_load_strings_const(const ERR_STRING_DATA *str);
#  endif
#  ifndef  ERR_LIB_HTTP
#   define ERR_LIB_HTTP (ERR_LIB_USER - 3)
int ossl_err_load_HTTP_strings(void);
#  endif
#  ifndef  ERR_LIB_CRMF
#   define ERR_LIB_CRMF (ERR_LIB_USER - 2)
int ossl_err_load_CRMF_strings(void);
#  endif
#  undef  CRMFerr
#  define CRMFerr(f, r) ERR_PUT_error(ERR_LIB_CRMF, 0, (r), __FILE__, __LINE__)
#  ifndef  ERR_LIB_CMP
#   define ERR_LIB_CMP (ERR_LIB_USER - 1)
int ossl_err_load_CMP_strings(void);
#  endif
#  undef  CMPerr
#  define CMPerr(f, r) ERR_PUT_error(ERR_LIB_CMP, 0, (r), __FILE__, __LINE__)
# endif

# include <openssl/safestack.h>
# if OPENSSL_VERSION_NUMBER >= 0x10101000L || defined(CMP_STANDALONE)
#  include <openssl/crmferr.h>
# endif
# ifndef  DECLARE_ASN1_DUP_FUNCTION
#  define DECLARE_ASN1_DUP_FUNCTION(type) \
    DECLARE_ASN1_DUP_FUNCTION_name(type, type)
#  define DECLARE_ASN1_DUP_FUNCTION_name(type, name) type *name##_dup(type *a);
#  define X509_NAME_set(xn, name) X509_NAME_set((xn), (X509_NAME *)(name))
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100000L
typedef unsigned char uint8_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;
#  define ERR_R_PASSED_INVALID_ARGUMENT CRMF_R_NULL_ARGUMENT
#  define int64_t long
#  define ASN1_INTEGER_get_int64(var, a) ((*(var) = ASN1_INTEGER_get(a)) != -1)
#  define ASN1_INTEGER_set_int64(a, v) ASN1_INTEGER_set(a, v)
#  define static_ASN1_SEQUENCE_END(T) ASN1_SEQUENCE_END(T)
#  define ASN1_R_TOO_SMALL ASN1_R_INVALID_NUMBER
#  define ASN1_R_TOO_LARGE ASN1_R_INVALID_NUMBER
#  define X509_ALGOR_cmp(a, b) (OBJ_cmp((a)->algorithm, (b)->algorithm) ? \
                                OBJ_cmp((a)->algorithm, (b)->algorithm) :\
                                (!(a)->parameter && !(b)->parameter) ? 0 : \
                                ASN1_TYPE_cmp(a->parameter, b->parameter))
#  define X509_V_FLAG_NO_CHECK_TIME 0x200000
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100002L
#  define EVP_MD_CTX_new()      EVP_MD_CTX_create()
#  define EVP_MD_CTX_reset(ctx) EVP_MD_CTX_init((ctx))
#  define EVP_MD_CTX_free(ctx)  EVP_MD_CTX_destroy((ctx))
#  define OPENSSL_clear_free(addr, num) OPENSSL_free(((void)(num), addr))
size_t OPENSSL_strlcpy(char *dst, const char *src, size_t siz);
#  ifndef CMP_STANDALONE
#   define DEFINE_STACK_OF DECLARE_STACK_OF
#  endif
# endif

# if OPENSSL_VERSION_NUMBER < 0x10100005L
#  define X509_PUBKEY_get0(x)((x)->pkey)
# endif
# if OPENSSL_VERSION_NUMBER < 0x10101000L
int X509_STORE_add_cert_dups(X509_STORE *ctx, X509 *x);
#  define X509_STORE_add_cert(ctx, x) X509_STORE_add_cert_dups(ctx, x)
#  define OBJ_obj2nid(alg) \
    (OBJ_obj2nid(alg) == NID_hmac_md5  ? NID_hmacWithMD5  : \
     OBJ_obj2nid(alg) == NID_hmac_sha1 ? NID_hmacWithSHA1 : OBJ_obj2nid(alg))
    /*
     * OID 1.3.6.1.5.5.8.1.2 associated with NID_hmac_sha1 is explicitly
     * mentioned in RFC 4210 and RFC 3370, but NID_hmac_sha1 is not included in
     * builitin_pbe[] of crypto/evp/evp_pbe.c
     */
    /*
     * NID_hmac_md5 not included in builtin_pbe[] of crypto/evp/evp_pbe.c as
     * it is not explicitly referenced in the RFC it might not be used by any
     * implementation although its OID 1.3.6.1.5.5.8.1.1 it is in the same OID
     * branch as NID_hmac_sha1
     */
# endif

# ifdef CMP_STANDALONE
#  include <openssl/err.h>
# endif
# if OPENSSL_VERSION_NUMBER >= 0x10101000L || defined(CMP_STANDALONE)
#  include <openssl/cmperr.h>
# endif

# if OPENSSL_VERSION_NUMBER < 0x10101000L
#  define X509_V_ERR_OCSP_VERIFY_NEEDED 73 /* Need OCSP verification */
#  define X509_V_ERR_OCSP_VERIFY_FAILED 74 /* Could not verify cert via OCSP */
# endif
# if OPENSSL_VERSION_NUMBER >= 0x10100007L
#  define OPENSSL_CMP_CONST const
# else
#  define OPENSSL_CMP_CONST
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100004L
#  define OPENSSL_die(msg, file, line) OpenSSLDie(file, line, msg)
#  define OPENSSL_FILE __FILE__
#  define OPENSSL_LINE __LINE__
#  define BIO_up_ref(b)((b)->references++)
# endif
# if OPENSSL_VERSION_NUMBER >= 0x30000000L
#  define OpenSSL_version_num() ((unsigned long) \
                                 ((OPENSSL_version_major() << 28) \
                                  | (OPENSSL_version_minor() << 20) \
                                  | (OPENSSL_version_patch() << 4L) \
                                  | _OPENSSL_VERSION_PRE_RELEASE))
# else
#  define ERR_raise(lib, r) \
    ERR_PUT_error((lib), 0, (r), OPENSSL_FILE, OPENSSL_LINE)
#  define ERR_raise_data \
    (ERR_set_debug(OPENSSL_FILE, OPENSSL_LINE, OPENSSL_FUNC), ERR_raise_data_)
void ERR_set_debug(const char *file, int line, const char *func);
void ERR_raise_data_(int lib, int reason, const char *fmt, ...);
#  define ERR_R_UNSUPPORTED (7|ERR_R_FATAL) /* dummy */
#  define ERR_SYSTEM_FLAG ((unsigned int)INT_MAX + 1)
#  define OSSL_CMP_DEFAULT_PORT 80
int OSSL_CMP_load_cert_crl_http_timeout(const char *url, int req_timeout,
                                        X509 **pcert, X509_CRL **pcrl,
                                        BIO *bio_err);
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100000L
#  define OpenSSL_version_num SSLeay
#  define X509_get0_subject_key_id(x) (X509_check_purpose((x), -1, -1), \
                                       (x)->skid)
#  define OPENSSL_strndup strndup
#  define SSL_AD_REASON_OFFSET 1000
#  define TLS1_AD_UNKNOWN_CA 48
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100005L
#  define X509_REQ_get0_pubkey(x) X509_PUBKEY_get0((x)->req_info->pubkey)
char *OPENSSL_buf2hexstr(const unsigned char *buffer, long len);
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100006L
#  define X509_get0_pubkey(x) X509_PUBKEY_get0((x)->cert_info->key)
#  ifndef X509_STORE_CTX_get1_crls
#   define X509_STORE_CTX_get1_crls X509_STORE_get1_crls
#  endif
#  define X509_STORE_lock(store)   /* sorry, no-op */
#  define X509_STORE_unlock(store) /* sorry, no-op */
#  define EVP_PKEY_up_ref(x)((x)->references++)
typedef int (*X509_STORE_CTX_check_revocation_fn) (X509_STORE_CTX *ctx);
#  ifdef CMP_STANDALONE
DECLARE_STACK_OF(ASN1_UTF8STRING)
#  endif
# endif
# if OPENSSL_VERSION_NUMBER < 0x10100007L
# include <limits.h>
#  define X509_ALGOR_get0(paobj, pptype, ppval, algor) \
    X509_ALGOR_get0((ASN1_OBJECT **)paobj, pptype, (void **)ppval, algor)
#  define X509_get0_notBefore X509_get_notBefore
#  define X509_get0_notAfter X509_get_notAfter
#  define X509_get_issuer_name(x) ((x)->cert_info->issuer)
#  define X509_get0_serialNumber(x) ((x)->cert_info->serialNumber)
#  define X509_get0_extensions(x) ((x)->cert_info->extensions)
# endif
# if OPENSSL_VERSION_NUMBER < 0x1010001fL && !defined(OPENSSL_zalloc)
#  define OPENSSL_zalloc(num) CRYPTO_zalloc(num, __FILE__, __LINE__)
#  include <string.h>
static inline void *CRYPTO_zalloc(size_t num, const char *file, int line)
{
    void *ret = CRYPTO_malloc((int)num, file, line);
    if (ret)
        memset(ret, 0, num);
    return ret;
}
#  define X509_up_ref(x)((x)->references++)
#  define X509_STORE_up_ref(x)((x)->references++)
#  define ASN1_STRING_get0_data ASN1_STRING_data
#  define X509_OBJECT_get0_X509(obj) ((obj) == NULL || \
                                      (obj)->type != X509_LU_X509 ? NULL : \
                                      (obj)->data.x509)
#  define X509_STORE_get0_objects(store) ((store)->objs)
#  define X509_STORE_CTX_get0_untrusted(ctx) ((ctx)->untrusted)
#  define X509_STORE_CTX_get0_chain X509_STORE_CTX_get_chain
#  define X509_STORE_CTX_get_by_subject X509_STORE_get_by_subject
#  define X509_STORE_CTX_set_current_cert(ctx, x) { (ctx)->current_cert = (x); }
#  define X509_STORE_CTX_set_error_depth(ctx, n) { (ctx)->error_depth = (n); }
typedef int (*X509_STORE_CTX_verify_cb)(int, X509_STORE_CTX *);
#  define X509_STORE_CTX_get_verify_cb(ctx) ((ctx)->verify_cb)
#  define X509_STORE_CTX_set0_verified_chain(ctx, sk) \
    { \
        sk_X509_pop_free((ctx)->chain, X509_free); \
        (ctx)->chain = (sk); \
    }
#  define X509_STORE_CTX_get_check_revocation(ctx) ((ctx)->check_revocation)
#  define X509_STORE_get_verify_cb(store) ((store)->verify_cb)
#  define X509_STORE_get0_param(ctx) ((ctx)->param)
#  define X509_STORE_set_ex_data(ctx, idx, data) \
    CRYPTO_set_ex_data(&(ctx)->ex_data, (idx), (data))
#  define X509_STORE_get_ex_data(ctx, idx) \
    CRYPTO_get_ex_data(&(ctx)->ex_data, (idx))
#  define X509_STORE_get_check_revocation(st) ((st)->check_revocation)
#  define X509_STORE_set_check_revocation(s, f) {(s)->check_revocation = (f); }
#  if OPENSSL_VERSION_NUMBER < 0x10002090L
#   define X509_V_ERR_STORE_LOOKUP 70 /* from x509_vfy.h */
#  endif
#  define X509_STORE_set_lookup_crls X509_STORE_set_lookup_crls_cb
#  define X509_VERIFY_PARAM_get_time(param) ((param)->check_time)
#  define X509_set_proxy_flag(x) { (x)->ex_flags |= EXFLAG_PROXY; }
#  define X509_CRL_get0_lastUpdate X509_CRL_get_lastUpdate
#  define X509_CRL_get0_nextUpdate X509_CRL_get_nextUpdate
#  define X509_get_key_usage(x) ((X509_check_purpose((x), -1, -1), \
                                  (x)->ex_flags & EXFLAG_KUSAGE) ? \
                                 (x)->ex_kusage : (unsigned long) ~0)
#  define TLS_client_method SSLv23_client_method
# endif

# if OPENSSL_VERSION_NUMBER < 0x30000000L
#  define SKM_DEFINE_STACK_OF_INTERNAL(t1, t2, t3) \
    STACK_OF(t1); \
    typedef int (*sk_##t1##_compfunc)(const t3 * const *a, const t3 *const *b); \
    typedef void (*sk_##t1##_freefunc)(t3 *a); \
    typedef t3 * (*sk_##t1##_copyfunc)(const t3 *a); \
    static ossl_unused ossl_inline t2 *ossl_check_##t1##_type(t2 *ptr) \
    { \
        return ptr; \
    } \
    static ossl_unused ossl_inline const OPENSSL_STACK \
    *ossl_check_const_##t1##_sk_type(const STACK_OF(t1) *sk) \
    { \
        return (const OPENSSL_STACK *)sk; \
    } \
    static ossl_unused ossl_inline OPENSSL_STACK \
    *ossl_check_##t1##_sk_type(STACK_OF(t1) *sk) \
    { \
        return (OPENSSL_STACK *)sk; \
    } \
    static ossl_unused ossl_inline OPENSSL_sk_compfunc \
    ossl_check_##t1##_compfunc_type(sk_##t1##_compfunc cmp) \
    { \
        return (OPENSSL_sk_compfunc)cmp; \
    } \
    static ossl_unused ossl_inline OPENSSL_sk_copyfunc \
    ossl_check_##t1##_copyfunc_type(sk_##t1##_copyfunc cpy) \
    { \
        return (OPENSSL_sk_copyfunc)cpy; \
    } \
    static ossl_unused ossl_inline OPENSSL_sk_freefunc \
    ossl_check_##t1##_freefunc_type(sk_##t1##_freefunc fr) \
    { \
        return (OPENSSL_sk_freefunc)fr; \
    }

typedef void OSSL_LIB_CTX;
DECLARE_ASN1_DUP_FUNCTION(X509_PUBKEY)
#  define OSSL_DEPRECATEDIN_3_0

    BIO *ASN1_item_i2d_mem_bio(const ASN1_ITEM *it, const ASN1_VALUE *val);
#  define X509_new_ex(libctx, propq) ((void)(libctx), (void)(propq), X509_new())
#  define d2i_PrivateKey_ex_bio(bp, a, libctx, propq) \
    ((void)(libctx), (void)(propq), d2i_PrivateKey_bio(bp, a))
#  define d2i_AutoPrivateKey_ex(a, pp, length, libctx, propq) \
    ((void)(libctx), (void)(propq), d2i_AutoPrivateKey(a, pp, length))
#  define ASN1_item_new_ex(it, l, pq) ((void)(l), (void)(pq), ASN1_item_new(it))
#  define ASN1_item_d2i_ex(a, in, len, it, libctx, propq) \
    ((void)(libctx), (void)(propq), ASN1_item_d2i(a, in, len, it))
#  define ASN1_item_d2i_bio_ex(it, in, pval, libctx, propq) \
    ((void)(libctx), (void)(propq), ASN1_item_d2i_bio(it, in, pval))
#  define ASN1_item_d2i_bio(it, in, pval) \
    ((in) == NULL ? NULL : ASN1_item_d2i_bio(it, in, pval)) /* hack */
#  define ASN1_item_sign_ex(it, algor1, algor2, signature, data, \
                            id, pkey, md, libctx, propq) \
    ASN1_item_sign(it, algor1, algor2, signature, (void *)(data), \
                   (EVP_PKEY *)pkey, ((void)(id), \
                                      (void)(libctx), (void)(propq), md))
#  define ASN1_item_verify_ex(it, alg, sig, data, id, pkey, libctx, propq) \
    ASN1_item_verify(it, alg, sig, data, \
                     ((void)(id), (void)(libctx), (void)(propq), pkey))
#  define X509_REQ_verify_ex(a, r, libctx, propq) \
    X509_REQ_verify(a, ((void)(libctx), (void)(propq), r))
#  define X509_PUBKEY_eq(a, b) ((a) == (b) || \
                                ((a) != NULL && (b) != NULL &&  \
                                 EVP_PKEY_cmp(X509_PUBKEY_get0(a), \
                                              X509_PUBKEY_get0(b)) == 0))
int EVP_PKEY_get_default_digest_name(EVP_PKEY *pkey,
                                     char *mdname, size_t mdname_sz);
#  define EVP_PKEY_CTX_new_from_pkey(libctx, pkey, propq) \
    EVP_PKEY_CTX_new(((void)(libctx), (void)(propq), pkey), NULL)
#  define CMS_ContentInfo_new_ex(libctx, propq) \
    ((void)(libctx), (void)(propq), CMS_ContentInfo_new())
#  define EVP_PKEY_get_id EVP_PKEY_id
#  define EVP_MD_get_type EVP_MD_type
#  define EVP_CIPHER_get_block_size EVP_CIPHER_block_size
#  define EVP_CIPHER_get_key_length EVP_CIPHER_key_length
#  define EVP_CIPHER_get_iv_length EVP_CIPHER_iv_length
#  define EVP_CIPHER_fetch(l, n, p) ((EVP_CIPHER *)EVP_get_cipherbyname(n))
#  define X509_STORE_CTX_new_ex(libctx, propq) \
    ((void)(libctx), (void)(propq), X509_STORE_CTX_new())
#  define RAND_bytes_ex(ctx, buf, num, x) RAND_bytes(((void)(ctx), buf), num)
int OSSL_HTTP_proxy_connect(BIO *bio, const char *server, const char *port,
                            const char *proxyuser, const char *proxypass,
                            int timeout, BIO *bio_err, const char *prog);
#  define X509_ADD_FLAG_DEFAULT  0
#  define X509_ADD_FLAG_UP_REF   0x1
#  define X509_ADD_FLAG_PREPEND  0x2
#  define X509_ADD_FLAG_NO_DUP   0x4
#  define X509_ADD_FLAG_NO_SS    0x8
#  define OPENSSL_NO_TRACE
#  define OSSL_TRACE_ENABLED(cat) 0
#  define OSSL_TRACE(cat, msg)          ((void)1)
#  define OSSL_TRACE1(cat, msg, a1)     ((void)1)
#  define OSSL_TRACE2(cat, msg, a1, a2) ((void)1)
#  define EVP_MD_fetch(ctx, alg, prop) \
    ((EVP_MD *)EVP_get_digestbynid(((void)ctx, (void)(prop), OBJ_sn2nid(alg))))
#  define EVP_MD_free(md) OPENSSL_free(((void)(md), NULL))
#  ifndef OPENSSL_FUNC
#   if defined(__STDC_VERSION__)
#    if __STDC_VERSION__ >= 199901L
#     define OPENSSL_FUNC __func__
#    elif defined(__GNUC__) && __GNUC__ >= 2
#     define OPENSSL_FUNC __FUNCTION__
#    endif
#   elif defined(_MSC_VER)
#    define OPENSSL_FUNC __FUNCTION__
#   endif
/*
 * If all these possibilities are exhausted, we give up and use a
 * static string.
 */
#   ifndef OPENSSL_FUNC
#    define OPENSSL_FUNC "(unknown function)"
#   endif
#  endif
#  define X509_FLAG_EXTENSIONS_ONLY_KID (1L << 13)
#  define OCSP_REVOKED_STATUS_AACOMPROMISE 10

#  define X509_R_UNKNOWN_SIGID_ALGS 144
#  define CMP_R_CONNECT_TIMEOUT CMP_R_TOTAL_TIMEOUT
#  define CMP_R_READ_TIMEOUT CMP_R_TOTAL_TIMEOUT
#  define CMP_R_ERROR_CONNECTING CMP_R_TRANSFER_ERROR
#  define CMP_R_FAILED_TO_SEND_REQUEST CMP_R_TRANSFER_ERROR
#  define CMP_R_TLS_ERROR CMP_R_TRANSFER_ERROR
#  define CMP_R_FAILED_TO_RECEIVE_PKIMESSAGE CMP_R_TRANSFER_ERROR
#  define CMP_R_ERROR_DECODING_MESSAGE CMP_R_TRANSFER_ERROR
#  define BIO_R_CONNECT_TIMEOUT BIO_R_CONNECT_ERROR
#  define BIO_R_TRANSFER_TIMEOUT BIO_R_CONNECT_ERROR
#  define BIO_R_TRANSFER_ERROR BIO_R_CONNECT_ERROR
#  define ERR_SYSTEM_ERROR(err) 0
#  define ERR_add_error_txt(sep, txt) ossl_cmp_add_error_txt(sep, txt)
#  define OSSL_CMP_add_error_data(txt) ERR_add_error_txt(" : ", txt)
#  define ERR_peek_error_data(data, flags) \
    ERR_peek_error_line_data(NULL, NULL, data, flags)
#  define ERR_peek_last_error_all(file, line, func, data, flags) \
    ERR_peek_last_error_line_data(file, line, ((*func = ""), data), flags)
#  define ERR_get_error_all(file, line, func, data, flags) \
    ERR_get_error_line_data(file, line, ((*func = ""), data), flags)
#  define OSSL_trace_set_channel(cat, bio) (BIO_free(bio), 1)
void ERR_add_error_mem_bio(const char *separator, BIO *bio);

ASN1_OCTET_STRING *X509_digest_sig(const X509 *cert,
                                   EVP_MD **md_used, int *md_is_fallback);
#  if !defined(OPENSSL_NO_OCSP) && !defined(OPENSSL_NO_SOCK)
int BIO_wait(BIO *bio, time_t max_time, unsigned int nap_milliseconds);
int BIO_do_connect_retry(BIO *bio, int timeout, int nap_milliseconds);
#  endif

STACK_OF(X509) *X509_build_chain(X509 *target, STACK_OF(X509) *certs,
                                 X509_STORE *store, int with_self_signed,
                                 OSSL_LIB_CTX *libctx, const char *propq);
#  define X509_chain_up_ref(c) (c == NULL ? c: X509_chain_up_ref(c)) /* hack */
#  define ASN1_item_i2d_bio(it, res, val) \
    ASN1_item_i2d_bio(it, res, (ASN1_VALUE *)(val)) /* hack */
#  define ASN1_VALUE_dup(a) ASN1_VALUE_dup((ASN1_VALUE *)(a)) /* hack */
#  define X509_REQ_dup(r) X509_REQ_dup((X509_REQ *)(r)) /* hack */
#  define X509_NAME_dup(n) X509_NAME_dup((X509_NAME *)(n)) /* hack */
#  define GENERAL_NAME_dup(n) GENERAL_NAME_dup((GENERAL_NAME *)(n)) /* hack */

char *ossl_sk_ASN1_UTF8STRING2text(STACK_OF(ASN1_UTF8STRING) *text,
                                   const char *sep,
                                   size_t max_len /* excl. NUL terminator */);
int X509_cmp_timeframe(const X509_VERIFY_PARAM *vpm,
                       const ASN1_TIME *start, const ASN1_TIME *end);
STACK_OF(X509) *X509_STORE_get1_all_certs(X509_STORE *store);
int X509_self_signed(X509 *cert, int verify_signature);
int X509_add_cert(STACK_OF(X509) *sk, X509 *cert, int flags);
int X509_add_certs(STACK_OF(X509) *sk, STACK_OF(X509) *certs, int flags);
typedef struct ossl_http_req_ctx_st OSSL_HTTP_REQ_CTX;
#  define ASN1_OP_DUP_POST -1 /* dummy */
#  define ASN1_OP_GET0_LIBCTX -2 /* dummy */
#  define ASN1_OP_GET0_PROPQ -3 /* dummy */
# endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */

/* OpenSSL-internal functions: */

/*  from crypto/ctype.h: */
# include <ctype.h>
# define ossl_isspace isspace
# define ossl_isprint isprint
void ossl_cmp_add_error_txt(const char *separator, const char *txt);
/* from crypto/x509.h: */
int ossl_x509v3_cache_extensions(X509 *x);
int ossl_x509_print_ex_brief(BIO *bio, X509 *cert, unsigned long neg_cflags);
int ossl_cmp_sk_X509_add1_cert(STACK_OF(X509) *sk, X509 *cert,
                               int no_dup, int prepend);
int ossl_cmp_sk_X509_add1_certs(STACK_OF(X509) *sk, STACK_OF(X509) *certs,
                                int no_self_signed, int no_dups, int prepend);
int ossl_x509_add_certs_new(STACK_OF(X509) **p_sk, STACK_OF(X509) *certs,
                            int flags);
int ossl_x509_add_cert_new(STACK_OF(X509) **p_sk, X509 *cert, int flags);
/* from crypto/bio.h: */
int BIO_get_line(BIO *bio, char *buf, int size);

# if OPENSSL_VERSION_NUMBER < 0x30000000L
#  define ossl_x509_set0_libctx(crt, libctx, propq) 0 /* dummy */
# else /* OPENSSL_VERSION_NUMBER >= 0x30000000L */

/* from crypto/x509.h: */
int ossl_x509_set0_libctx(X509 *x, OSSL_LIB_CTX *libctx, const char *propq);
/* from include/internal/cryptlib.h */
int openssl_strerror_r(int errnum, char *buf, size_t buflen);
# endif /* OPENSSL_VERSION_NUMBER < 0x30000000L */

#endif /* OPENSSL_BACKPORT_H */
