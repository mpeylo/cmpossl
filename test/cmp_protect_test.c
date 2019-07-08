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
 * CMP tests by Martin Peylo, Tobias Pankert, and David von Oheimb.
 */

#include "cmp_testlib.h"

static const char *server_f;
static const char *ir_protected_f;
static const char *ir_unprotected_f;
static const char *ip_PBM_f;
static const char *server_key_f;
static const char *server_cert_f;
static const char *endentity1_f;
static const char *endentity2_f;
static const char *root_f;
static const char *intermediate_f;

/* Add test code as per
 * http://wiki.openssl.org/index.php/How_To_Write_Unit_Tests_For_OpenSSL#Style
 */
typedef struct test_fixture {
    const char *test_case_name;
    OSSL_CMP_CTX *cmp_ctx;
    /* for protection tests */
    OSSL_CMP_MSG *msg;
    OSSL_CMP_PKISI *si;      /* for error and response messages */
    ASN1_OCTET_STRING *secret;
    EVP_PKEY *privkey;
    EVP_PKEY *pubkey;
    unsigned char *mem;
    int memlen;
    X509 *cert;
    STACK_OF(X509) *certs;
    STACK_OF(X509) *chain;
    int callback_arg;
    int expected;
} CMP_PROTECT_TEST_FIXTURE;

static CMP_PROTECT_TEST_FIXTURE *set_up(const char *const test_case_name)
{
    CMP_PROTECT_TEST_FIXTURE *fixture;
    int setup_ok = 0;

    /* Allocate memory owned by the fixture, exit on error */
    if (!TEST_ptr(fixture = OPENSSL_zalloc(sizeof(*fixture))))
        goto err;
    fixture->test_case_name = test_case_name;

    if (!TEST_ptr(fixture->cmp_ctx = OSSL_CMP_CTX_new()))
        goto err;

    setup_ok = 1;
 err:
    if (!setup_ok) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return fixture;
}

static void tear_down(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    /* ERR_print_errors_fp(stderr);
       Free any memory owned by the fixture, etc. */
    OSSL_CMP_CTX_free(fixture->cmp_ctx);
    OSSL_CMP_MSG_free(fixture->msg);
    ASN1_OCTET_STRING_free(fixture->secret);
    EVP_PKEY_free(fixture->privkey);
    EVP_PKEY_free(fixture->pubkey);
    OSSL_CMP_PKISI_free(fixture->si);

    OPENSSL_free(fixture->mem);
    sk_X509_free(fixture->certs);
    sk_X509_free(fixture->chain);

    OPENSSL_free(fixture);
}

static EVP_PKEY *loadedprivkey = NULL;
static EVP_PKEY *loadedpubkey = NULL;
static EVP_PKEY *loadedkey = NULL;
static X509 *cert = NULL;
static unsigned char rand_data[OSSL_CMP_TRANSACTIONID_LENGTH];
static OSSL_CMP_MSG *ir_unprotected, *ir_protected;
static X509 *endentity1 = NULL, *endentity2 = NULL,
    *root = NULL, *intermediate = NULL;

static int execute_calc_protection_fails_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    ASN1_BIT_STRING *protection =
        CMP_calc_protection(fixture->msg, fixture->secret, fixture->privkey);
    int res = TEST_ptr_null(protection);

    ASN1_BIT_STRING_free(protection);
    return res;
}

/* TODO internal test*/
static int execute_calc_protection_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    ASN1_BIT_STRING *protection =
        CMP_calc_protection(fixture->msg, fixture->secret, fixture->privkey);
    int res = TEST_ptr(protection)
                  && TEST_true(ASN1_STRING_cmp(protection,
                                               fixture->msg->protection) == 0);

    ASN1_BIT_STRING_free(protection);
    return res;
}

/* This function works similar to parts of CMP_verify_signature in cmp_vfy.c,
 * but without the need for a OSSL_CMP_CTX or a X509 certificate */
static int verify_signature(OSSL_CMP_MSG *msg,
                            ASN1_BIT_STRING *protection,
                            EVP_PKEY *pkey, int digest_nid)
{
    CMP_PROTECTEDPART prot_part;
    unsigned char *prot_part_der = NULL;
    int l;
    EVP_MD_CTX *ctx = NULL;
    int res;

    prot_part.header = OSSL_CMP_MSG_get0_header(msg);
    prot_part.body = msg->body;
    res =
        TEST_int_ge(l = i2d_CMP_PROTECTEDPART(&prot_part, &prot_part_der), 0)
            && TEST_ptr(ctx = EVP_MD_CTX_create())
            && TEST_true(EVP_VerifyInit_ex
                         (ctx, (EVP_MD *)EVP_get_digestbynid(digest_nid), NULL))
            && TEST_true(EVP_VerifyUpdate(ctx, prot_part_der, l))
            && TEST_int_eq(EVP_VerifyFinal(ctx, protection->data,
                                           protection->length, pkey), 1);
    /* cleanup */
    EVP_MD_CTX_destroy(ctx);
    OPENSSL_free(prot_part_der);
    return res;
}

/* Calls OSSL_CMP_calc_protection and verifies signature*/
static int execute_calc_protection_signature_test(CMP_PROTECT_TEST_FIXTURE *
                                                  fixture)
{
    ASN1_BIT_STRING *protection =
        CMP_calc_protection(fixture->msg, NULL, fixture->privkey);
    int ret = (TEST_ptr(protection)
                   && TEST_true(verify_signature(fixture->msg, protection,
                                                 fixture->pubkey,
                                                 fixture->cmp_ctx->digest)));

    ASN1_BIT_STRING_free(protection);
    return ret;
}

/* TODO TPa: find a way to set protection algorithm */
static int test_cmp_calc_protection_no_key_no_secret(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    /* Do test case-specific set up; set expected return values and
     * side effects */
    if (!TEST_ptr(fixture->msg = load_pkimsg(ir_unprotected_f))
         || !TEST_ptr(fixture->msg->header->protectionAlg = X509_ALGOR_new())) {
        tear_down(fixture);
        fixture = NULL;
    }

    EXECUTE_TEST(execute_calc_protection_fails_test, tear_down);
    return result;
}

/* TODO TPa: find openssl-independent reference value */
static int test_cmp_calc_protection_pkey(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->pubkey = loadedpubkey;
    fixture->privkey = loadedprivkey;
    if (!TEST_true(EVP_PKEY_up_ref(loadedpubkey))
            || !TEST_true(EVP_PKEY_up_ref(loadedprivkey))
            || !TEST_ptr(fixture->msg = load_pkimsg(ir_protected_f))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_calc_protection_signature_test, tear_down);
    return result;
}

static int test_cmp_calc_protection_pbmac(void)
{
    unsigned char sec_insta[] = { 'i', 'n', 's', 't', 'a' };

    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    if (!TEST_ptr(fixture->secret = ASN1_OCTET_STRING_new())
            || !TEST_true(ASN1_OCTET_STRING_set
                          (fixture->secret, sec_insta, sizeof(sec_insta)))
            || !TEST_ptr(fixture->msg = load_pkimsg(ip_PBM_f))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_calc_protection_test, tear_down);
    return result;
}
static int execute_MSG_protect_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    return TEST_int_eq(fixture->expected,
                       OSSL_CMP_MSG_protect(fixture->cmp_ctx, fixture->msg));
}

static int test_MSG_protect_unprotected_request(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    /* Do test case-specific set up; set expected return values and
     * side effects */
    fixture->expected = 1;
    if (!TEST_ptr(fixture->msg =
                  OSSL_CMP_MSG_dup(ir_unprotected))
            || !TEST_true(OSSL_CMP_CTX_set_option(fixture->cmp_ctx,
                                           OSSL_CMP_OPT_UNPROTECTED_SEND, 1))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_protect_test, tear_down);
    return result;
}

static int test_MSG_protect_with_msg_sig_alg_protection_plus_rsa_key(void)
{
    const size_t size = sizeof(rand_data) / 2;

    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->expected = 1;

    if (!TEST_ptr(fixture->msg =
                  OSSL_CMP_MSG_dup(ir_unprotected))
            || !TEST_true(OSSL_CMP_CTX_set_option(fixture->cmp_ctx,
                                              OSSL_CMP_OPT_UNPROTECTED_SEND, 0))
        /* Use half of the 16 bytes of random input
         * for each reference and secret value */
            || !TEST_true(OSSL_CMP_CTX_set1_referenceValue(fixture->cmp_ctx,
                                                           rand_data, size))
            || !TEST_true(OSSL_CMP_CTX_set1_secretValue(fixture->cmp_ctx,
                                                        rand_data + size,
                                                        size))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_protect_test, tear_down);
    return result;
}

static int test_MSG_protect_with_certificate_and_key(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    /* Do test case-specific set up; set expected return values and
     * side effects */

    fixture->expected = 1;

    if (!TEST_ptr(fixture->msg =
                  OSSL_CMP_MSG_dup(ir_unprotected))
            || !TEST_true(OSSL_CMP_CTX_set_option(fixture->cmp_ctx,
                                              OSSL_CMP_OPT_UNPROTECTED_SEND, 0))
            || !TEST_true(OSSL_CMP_CTX_set1_pkey(fixture->cmp_ctx, loadedkey))
            || !TEST_true(OSSL_CMP_CTX_set1_clCert(fixture->cmp_ctx, cert))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_protect_test, tear_down);
    return result;
}

static int test_MSG_protect_certificate_based_without_cert(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    /* Do test case-specific set up; set expected return values and
     * side effects */
    fixture->expected = 0;
    if (!TEST_ptr(fixture->msg =
                  OSSL_CMP_MSG_dup(ir_unprotected))
        || !TEST_true(OSSL_CMP_CTX_set_option(fixture->cmp_ctx,
                                              OSSL_CMP_OPT_UNPROTECTED_SEND, 0))
        || !TEST_true(OSSL_CMP_CTX_set1_newPkey(fixture->cmp_ctx, loadedkey))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_protect_test, tear_down);
    return result;
}

static int test_MSG_protect_no_key_no_secret(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    /* Do test case-specific set up; set expected return values and
     * side effects */
    fixture->expected = 0;
    if (!TEST_ptr(fixture->msg = OSSL_CMP_MSG_dup(ir_unprotected))
            || !TEST_true(OSSL_CMP_CTX_set_option(fixture->cmp_ctx,
                                           OSSL_CMP_OPT_UNPROTECTED_SEND, 0))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_protect_test, tear_down);
    return result;
}

static int execute_MSG_add_extraCerts_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    return TEST_true(OSSL_CMP_MSG_add_extraCerts(fixture->cmp_ctx,
                                                   fixture->msg));
}

static int test_MSG_add_extraCerts(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    if (!TEST_ptr(fixture->msg = OSSL_CMP_MSG_dup(ir_protected))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_MSG_add_extraCerts_test, tear_down);
    return result;
}

static int execute_cmp_build_cert_chain_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    STACK_OF(X509) *result = NULL;
    int ret = 0;

    if (TEST_ptr(result = OSSL_CMP_build_cert_chain(fixture->certs,
                                                    fixture->cert))) {
        /* Check whether chain built is equal to the expected one */
        ret = TEST_int_eq(0, STACK_OF_X509_cmp(result, fixture->chain));
        sk_X509_pop_free(result, X509_free);
    }
    return ret;
}

static int test_cmp_build_cert_chain(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->cert = endentity2;
    if (!TEST_ptr(fixture->certs = sk_X509_new_null())
            || !TEST_ptr(fixture->chain = sk_X509_new_null())
            || !TEST_true(sk_X509_push(fixture->certs, endentity1))
            || !TEST_true(sk_X509_push(fixture->certs, root))
            || !TEST_true(sk_X509_push(fixture->certs, intermediate))
            || !TEST_true(sk_X509_push(fixture->chain, endentity2))
            || !TEST_true(sk_X509_push(fixture->chain, intermediate))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_cmp_build_cert_chain_test, tear_down);
    return result;
}

static int test_cmp_build_cert_chain_missing_intermediate(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->cert = endentity2;
    if (!TEST_ptr(fixture->certs = sk_X509_new_null())
            || !TEST_ptr(fixture->chain = sk_X509_new_null())
            || !TEST_true(sk_X509_push(fixture->certs, endentity1))
            || !TEST_true(sk_X509_push(fixture->certs, root))
            || !TEST_true(sk_X509_push(fixture->chain, endentity2))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_cmp_build_cert_chain_test, tear_down);
    return result;
}

static int test_cmp_build_cert_chain_missing_root(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->cert = endentity2;
    if (!TEST_ptr(fixture->certs = sk_X509_new_null())
            || !TEST_ptr(fixture->chain = sk_X509_new_null())
            || !TEST_true(sk_X509_push(fixture->certs, endentity1))
            || !TEST_true(sk_X509_push(fixture->certs, intermediate))
            || !TEST_true(sk_X509_push(fixture->chain, endentity2))
            || !TEST_true(sk_X509_push(fixture->chain, intermediate))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_cmp_build_cert_chain_test, tear_down);
    return result;
}

static int test_cmp_build_cert_chain_no_certs(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->cert = endentity2;
    if (!TEST_ptr(fixture->certs = sk_X509_new_null())
            || !TEST_ptr(fixture->chain = sk_X509_new_null())
            || !TEST_true(sk_X509_push(fixture->chain, endentity2))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_cmp_build_cert_chain_test, tear_down);
    return result;
}

static int execute_X509_STORE_test(CMP_PROTECT_TEST_FIXTURE *fixture)
{
    X509_STORE *store = X509_STORE_new();
    STACK_OF(X509) *sk = NULL;
    int res = 0;

    if (!TEST_true(OSSL_CMP_X509_STORE_add1_certs(store,
                                                  fixture->certs,
                                                  fixture->callback_arg)))
        goto err;
    sk = OSSL_CMP_X509_STORE_get1_certs(store);
    if (!TEST_int_eq(0, STACK_OF_X509_cmp(sk, fixture->chain)))
        goto err;
    res = 1;
 err:
    X509_STORE_free(store);
    sk_X509_pop_free(sk, X509_free);
    return res;

}

static int test_X509_STORE(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->certs = sk_X509_new_null();
    fixture->callback_arg = 0;  /* self-signed allowed */
    if (!TEST_true(sk_X509_push(fixture->certs, endentity1)
                       && sk_X509_push(fixture->certs, endentity2)
                       && sk_X509_push(fixture->certs, root)
                       && sk_X509_push(fixture->certs, intermediate))
            || !TEST_ptr(fixture->chain = sk_X509_dup(fixture->certs))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_X509_STORE_test, tear_down);
    return result;
}

static int test_X509_STORE_only_self_signed(void)
{
    SETUP_TEST_FIXTURE(CMP_PROTECT_TEST_FIXTURE, set_up);
    fixture->certs = sk_X509_new_null();
    fixture->chain = sk_X509_new_null();
    fixture->callback_arg = 1;  /* only self-signed */
    if (!TEST_true(sk_X509_push(fixture->certs, endentity1)
                       && sk_X509_push(fixture->certs, endentity2)
                       && sk_X509_push(fixture->certs, root)
                       && sk_X509_push(fixture->certs, intermediate))
            || !TEST_true(sk_X509_push(fixture->chain, root))) {
        tear_down(fixture);
        fixture = NULL;
    }
    EXECUTE_TEST(execute_X509_STORE_test, tear_down);
    return result;
}


void cleanup_tests(void)
{
    EVP_PKEY_free(loadedprivkey);
    EVP_PKEY_free(loadedpubkey);
    EVP_PKEY_free(loadedkey);
    X509_free(cert);
    X509_free(endentity1);
    X509_free(endentity2);
    X509_free(root);
    X509_free(intermediate);
    OSSL_CMP_MSG_free(ir_protected);
    OSSL_CMP_MSG_free(ir_unprotected);

}

int setup_tests(void)
{
    if (!TEST_ptr(server_f = test_get_argument(0))
            || !TEST_ptr(ir_protected_f = test_get_argument(1))
            || !TEST_ptr(ir_unprotected_f = test_get_argument(2))
            || !TEST_ptr(ip_PBM_f = test_get_argument(3))
            || !TEST_ptr(server_cert_f = test_get_argument(4))
            || !TEST_ptr(server_key_f = test_get_argument(5))
            || !TEST_ptr(endentity1_f = test_get_argument(6))
            || !TEST_ptr(endentity2_f = test_get_argument(7))
            || !TEST_ptr(root_f = test_get_argument(8))
            || !TEST_ptr(intermediate_f = test_get_argument(9))) {
        TEST_error("usage: cmp_protect_test server.pem "
                   "IR_protected.der IR_unprotected.der IP_PBM.der "
                   "server.crt server.pem"
                   "EndEntity1.crt EndEntity2.crt "
                   "Root_CA.crt Intermediate_CA.crt\n");
        return 0;
    }
    if (!TEST_ptr(loadedkey = load_pem_key(server_key_f))
            || !TEST_ptr(cert = load_pem_cert(server_cert_f)))
        return 0;

    if (!TEST_ptr(loadedprivkey = load_pem_key(server_f)))
        return 0;
    if (TEST_true(EVP_PKEY_up_ref(loadedprivkey)))
        loadedpubkey = loadedprivkey;
    if (!TEST_ptr(ir_protected = load_pkimsg(ir_protected_f))
            || !TEST_ptr(ir_unprotected = load_pkimsg(ir_unprotected_f)))
        return 0;
    if (!TEST_ptr(endentity1 = load_pem_cert(endentity1_f))
             || !TEST_ptr(endentity2 = load_pem_cert(endentity2_f))
             || !TEST_ptr(root = load_pem_cert(root_f))
             || !TEST_ptr(intermediate = load_pem_cert(intermediate_f)))
         return 0;
    if(!TEST_int_eq(1, RAND_bytes(rand_data, OSSL_CMP_TRANSACTIONID_LENGTH)))
        return 0;

    /* Message protection tests */
    ADD_TEST(test_cmp_calc_protection_no_key_no_secret);
    ADD_TEST(test_cmp_calc_protection_pkey);
    ADD_TEST(test_cmp_calc_protection_pbmac);

    ADD_TEST(test_MSG_protect_with_msg_sig_alg_protection_plus_rsa_key);
    ADD_TEST(test_MSG_protect_with_certificate_and_key);
    ADD_TEST(test_MSG_protect_certificate_based_without_cert);
    ADD_TEST(test_MSG_protect_unprotected_request);
    ADD_TEST(test_MSG_protect_no_key_no_secret);

    ADD_TEST(test_MSG_add_extraCerts);

    ADD_TEST(test_cmp_build_cert_chain);
    ADD_TEST(test_cmp_build_cert_chain_missing_root);
    ADD_TEST(test_cmp_build_cert_chain_missing_intermediate);
    ADD_TEST(test_cmp_build_cert_chain_no_certs);

    ADD_TEST(test_X509_STORE);
    ADD_TEST(test_X509_STORE_only_self_signed);

    return 1;
}
