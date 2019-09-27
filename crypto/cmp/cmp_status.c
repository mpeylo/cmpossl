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

/* CMP functions for PKIStatusInfo handling and PKIMessage decomposition */

#include <string.h>

#include "cmp_int.h"

/* explicit #includes not strictly needed since implied by the above: */
#include <time.h>
#include <openssl/cmp.h>
#include <openssl/crmf.h>
#include <openssl/err.h> /* needed in case config no-deprecated */
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/asn1err.h> /* for ASN1_R_TOO_SMALL and ASN1_R_TOO_LARGE */

/* CMP functions related to PKIStatus */

int ossl_cmp_pkisi_get_pkistatus(OSSL_CMP_PKISI *si)
{
    if (si == NULL || si->status == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return -1;
    }
    return ossl_cmp_asn1_get_int(si->status);
}


const char *ossl_cmp_PKIStatus_to_string(int status)
{
    switch (status) {
    case OSSL_CMP_PKISTATUS_accepted:
        return "PKIStatus: accepted";
    case OSSL_CMP_PKISTATUS_grantedWithMods:
        return "PKIStatus: granted with modifications";
    case OSSL_CMP_PKISTATUS_rejection:
        return "PKIStatus: rejection";
    case OSSL_CMP_PKISTATUS_waiting:
        return "PKIStatus: waiting";
    case OSSL_CMP_PKISTATUS_revocationWarning:
        return "PKIStatus: revocation warning - a revocation of the cert is imminent";
    case OSSL_CMP_PKISTATUS_revocationNotification:
        return "PKIStatus: revocation notification - a revocation of the cert has occurred";
    case OSSL_CMP_PKISTATUS_keyUpdateWarning:
        return "PKIStatus: key update warning - update already done for the cert";
    default:
        CMPerr(0, CMP_R_ERROR_PARSING_PKISTATUS);
        return NULL;
    }
}

/*
 * returns a pointer to the statusString contained in a PKIStatusInfo
 * returns NULL on error
 */
OSSL_CMP_PKIFREETEXT *ossl_cmp_pkisi_get0_statusstring(const OSSL_CMP_PKISI *si)
{
    if (si == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return NULL;
    }
    return si->statusString;
}

/*
 * returns the FailureInfo bits of the given PKIStatusInfo
 * returns -1 on error
 */
int ossl_cmp_pkisi_get_pkifailureinfo(OSSL_CMP_PKISI *si)
{
    int i;
    int res = 0;

    if (si == NULL || si->failInfo == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return -1;
    }
    for (i = 0; i <= OSSL_CMP_PKIFAILUREINFO_MAX; i++)
        if (ASN1_BIT_STRING_get_bit(si->failInfo, i))
            res |= 1 << i;
    return res;
}

/*
 * internal function
 * convert PKIFailureInfo number to human-readable string
 *
 * returns pointer to static string
 * returns NULL on error
 */
static const char *CMP_PKIFAILUREINFO_to_string(int number)
{
    switch (number) {
    case OSSL_CMP_PKIFAILUREINFO_badAlg:
        return "badAlg";
    case OSSL_CMP_PKIFAILUREINFO_badMessageCheck:
        return "badMessageCheck";
    case OSSL_CMP_PKIFAILUREINFO_badRequest:
        return "badRequest";
    case OSSL_CMP_PKIFAILUREINFO_badTime:
        return "badTime";
    case OSSL_CMP_PKIFAILUREINFO_badCertId:
        return "badCertId";
    case OSSL_CMP_PKIFAILUREINFO_badDataFormat:
        return "badDataFormat";
    case OSSL_CMP_PKIFAILUREINFO_wrongAuthority:
        return "wrongAuthority";
    case OSSL_CMP_PKIFAILUREINFO_incorrectData:
        return "incorrectData";
    case OSSL_CMP_PKIFAILUREINFO_missingTimeStamp:
        return "missingTimeStamp";
    case OSSL_CMP_PKIFAILUREINFO_badPOP:
        return "badPOP";
    case OSSL_CMP_PKIFAILUREINFO_certRevoked:
        return "certRevoked";
    case OSSL_CMP_PKIFAILUREINFO_certConfirmed:
        return "certConfirmed";
    case OSSL_CMP_PKIFAILUREINFO_wrongIntegrity:
        return "wrongIntegrity";
    case OSSL_CMP_PKIFAILUREINFO_badRecipientNonce:
        return "badRecipientNonce";
    case OSSL_CMP_PKIFAILUREINFO_timeNotAvailable:
        return "timeNotAvailable";
    case OSSL_CMP_PKIFAILUREINFO_unacceptedPolicy:
        return "unacceptedPolicy";
    case OSSL_CMP_PKIFAILUREINFO_unacceptedExtension:
        return "unacceptedExtension";
    case OSSL_CMP_PKIFAILUREINFO_addInfoNotAvailable:
        return "addInfoNotAvailable";
    case OSSL_CMP_PKIFAILUREINFO_badSenderNonce:
        return "badSenderNonce";
    case OSSL_CMP_PKIFAILUREINFO_badCertTemplate:
        return "badCertTemplate";
    case OSSL_CMP_PKIFAILUREINFO_signerNotTrusted:
        return "signerNotTrusted";
    case OSSL_CMP_PKIFAILUREINFO_transactionIdInUse:
        return "transactionIdInUse";
    case OSSL_CMP_PKIFAILUREINFO_unsupportedVersion:
        return "unsupportedVersion";
    case OSSL_CMP_PKIFAILUREINFO_notAuthorized:
        return "notAuthorized";
    case OSSL_CMP_PKIFAILUREINFO_systemUnavail:
        return "systemUnavail";
    case OSSL_CMP_PKIFAILUREINFO_systemFailure:
        return "systemFailure";
    case OSSL_CMP_PKIFAILUREINFO_duplicateCertReq:
        return "duplicateCertReq";
    default:
        return NULL; /* illegal failure number */
    }
}

/*
 * checks PKIFailureInfo bits in a given PKIStatusInfo
 * returns 1 if a given bit is set, 0 if not, -1 on error
 */
int ossl_cmp_pkisi_pkifailureinfo_check(OSSL_CMP_PKISI *si, int bit_index)
{
    if (si == NULL || si->failInfo == NULL) {
        CMPerr(0, CMP_R_NULL_ARGUMENT);
        return -1;
    }

    if (bit_index < 0 || bit_index > OSSL_CMP_PKIFAILUREINFO_MAX) {
        CMPerr(0, CMP_R_INVALID_ARGS);
        return -1;
    }

    return ASN1_BIT_STRING_get_bit(si->failInfo, bit_index);
}

/*
 * place human-readable error string created from PKIStatusInfo in given buffer
 * returns pointer to the same buffer containing the string, or NULL on error
 */
char *OSSL_CMP_CTX_snprint_PKIStatus(OSSL_CMP_CTX *ctx, char *buf, int bufsize)
{
    int status, failure, fail_info;
    const char *status_string, *failure_string;
    OSSL_CMP_PKIFREETEXT *status_strings;
    ASN1_UTF8STRING *text;
    int i;
    int n = 0;

    if (ctx == NULL || buf == NULL || bufsize <= 0
            || (status = OSSL_CMP_CTX_get_status(ctx)) < 0
            || (status_string = ossl_cmp_PKIStatus_to_string(status)) == NULL)
        return NULL;
    BIO_snprintf(buf, bufsize, "%s", status_string);

    /* failInfo is optional and may be empty */
    if ((fail_info = OSSL_CMP_CTX_get_failInfoCode(ctx)) > 0) {
        BIO_snprintf(buf + strlen(buf), bufsize - strlen(buf),
                     "; PKIFailureInfo: ");
        for (failure = 0; failure <= OSSL_CMP_PKIFAILUREINFO_MAX; failure++) {
            if ((fail_info & (1 << failure)) != 0) {
                failure_string = CMP_PKIFAILUREINFO_to_string(failure);
                if (failure_string != NULL) {
                    BIO_snprintf(buf + strlen(buf), bufsize - strlen(buf),
                                 "%s%s", n > 0 ? ", " : "", failure_string);
                    n += (int)strlen(failure_string);
                }
            }
        }
    }
    if (n == 0 && status != OSSL_CMP_PKISTATUS_accepted
            && status != OSSL_CMP_PKISTATUS_grantedWithMods)
        BIO_snprintf(buf + strlen(buf), bufsize - strlen(buf),
                     "; <no failure info>");

    /* statusString sequence is optional and may be empty */
    status_strings = OSSL_CMP_CTX_get0_statusString(ctx);
    n = sk_ASN1_UTF8STRING_num(status_strings);
    if (n > 0) {
        BIO_snprintf(buf + strlen(buf), bufsize - strlen(buf),
                     "; StatusString%s: ", n > 1 ? "s" : "");
        for (i = 0; i < n; i++) {
            text = sk_ASN1_UTF8STRING_value(status_strings, i);
            BIO_snprintf(buf + strlen(buf), bufsize - strlen(buf), "\"%s\"%s",
                         ASN1_STRING_get0_data(text), i < n - 1 ? ", " : "");
        }
    }
    return buf;
}

/*
 * Creates a new PKIStatusInfo structure and fills it in
 * returns a pointer to the structure on success, NULL on error
 * note: strongly overlaps with TS_RESP_CTX_set_status_info()
 * and TS_RESP_CTX_add_failure_info() in ../ts/ts_rsp_sign.c
 */
OSSL_CMP_PKISI *ossl_cmp_statusinfo_new(int status, int fail_info,
                                       const char *text)
{
    OSSL_CMP_PKISI *si = NULL;
    ASN1_UTF8STRING *utf8_text = NULL;
    int failure;

    if ((si = OSSL_CMP_PKISI_new()) == NULL)
        goto err;
    if (!ASN1_INTEGER_set(si->status, status))
        goto err;

    if (text != NULL) {
        if ((utf8_text = ASN1_UTF8STRING_new()) == NULL
                || !ASN1_STRING_set(utf8_text, text, (int)strlen(text)))
            goto err;
        if (si->statusString == NULL
                && (si->statusString = sk_ASN1_UTF8STRING_new_null()) == NULL)
            goto err;
        if (!sk_ASN1_UTF8STRING_push(si->statusString, utf8_text))
            goto err;
        /* Ownership is lost. */
        utf8_text = NULL;
    }

    for (failure = 0; failure <= OSSL_CMP_PKIFAILUREINFO_MAX; failure++) {
        if ((fail_info & (1 << failure)) != 0) {
            if (si->failInfo == NULL
                    && (si->failInfo = ASN1_BIT_STRING_new()) == NULL)
                goto err;
            if (!ASN1_BIT_STRING_set_bit(si->failInfo, failure, 1))
                goto err;
        }
    }
    return si;

 err:
    OSSL_CMP_PKISI_free(si);
    ASN1_UTF8STRING_free(utf8_text);
    return NULL;
}

