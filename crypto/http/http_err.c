/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include <openssl/httperr.h>

#ifndef OPENSSL_NO_ERR

static const ERR_STRING_DATA HTTP_str_reasons[] = {
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_ASN1_LEN_EXCEEDS_MAX_RESP_LEN),
    "asn1 len exceeds max resp len"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_CONNECT_FAILURE), "connect failure"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_ERROR_PARSING_ASN1_LENGTH),
    "error parsing asn1 length"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_ERROR_PARSING_URL), "error parsing url"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_ERROR_RECEIVING), "error receiving"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_ERROR_SENDING), "error sending"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_MISSING_ASN1_ENCODING),
    "missing asn1 encoding"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_MISSING_REDIRECT_LOCATION),
    "missing redirect location"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_REDIRECTION_FROM_HTTPS_TO_HTTP),
    "redirection from https to http"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_RESPONE_LINE_TOO_LONG),
    "respone line too long"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_SERVER_RESPONSE_PARSE_ERROR),
    "server response parse error"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_SERVER_SENT_ERROR), "server sent error"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_SERVER_SENT_WRONG_HTTP_VERSION),
    "server sent wrong http version"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_STATUS_CODE_UNSUPPORTED),
    "status code unsupported"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_TLS_NOT_SUPPORTED), "tls not supported"},
    {ERR_PACK(ERR_LIB_HTTP, 0, HTTP_R_TOO_MANY_REDIRECTIONS),
    "too many redirections"},
    {0, NULL}
};

#endif

int ERR_load_HTTP_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (ERR_reason_error_string(HTTP_str_reasons[0].error) == NULL)
        ERR_load_strings_const(HTTP_str_reasons);
#endif
    return 1;
}
