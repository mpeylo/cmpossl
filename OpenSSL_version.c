/*!*****************************************************************************
 * @file   OpenSSL_version.c
 * @brief  OpenSSL version checking utility
 * keep this file identical among genCMPClient and CMPforOpenSSL
 *
 * @author David von Oheimb, David.von.Oheimb@siemens.com
 *
 *  Copyright (c) 2021-2024 Siemens AG
 *  Licensed under the Apache License, Version 2.0
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <openssl/crypto.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
    #error Should not use OpenSSL versions before 3. They are unsupported and insecure.
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    #define OpenSSL_version_num SSLeay
    #define DEBUG_MESSAGE "[DEBUG] Using SSLeay() for OpenSSL version\n"
#elif OPENSSL_VERSION_NUMBER >= 0x30000000L
    #define OpenSSL_version_num() ((unsigned long) \
                                   ((OPENSSL_version_major()<<28) \
                                   |(OPENSSL_version_minor()<<20) \
                                   |(OPENSSL_version_patch()<< 4) \
                                   |_OPENSSL_VERSION_PRE_RELEASE ))
    #define DEBUG_MESSAGE "[DEBUG] Defining ourselves OpenSSL_version_num() for OpenSSL version\n"
#else
    #define DEBUG_MESSAGE "[DEBUG] Using existing OpenSSL_version_num() for OpenSSL version\n"

#endif

int main(int argc, char *argv[])
{
    fprintf(stderr, "[DEBUG] Starting OpenSSL version check\n");
    fprintf(stderr, DEBUG_MESSAGE);

    unsigned long static_version = (unsigned long)OPENSSL_VERSION_NUMBER;
    fprintf(stderr, "[TRACE] OPENSSL_VERSION_NUMBER:  0x%lx\n", static_version);

    unsigned long runtime_version = OpenSSL_version_num();
    fprintf(stderr, "[TRACE] OpenSSL runtime_version: 0x%lx\n", runtime_version);

    #define MAJOR_MINOR_MASK 0xfff00000L
    if ((MAJOR_MINOR_MASK & runtime_version ) != (MAJOR_MINOR_MASK & OPENSSL_VERSION_NUMBER)) {
        fprintf(stderr, "OpenSSL runtime version 0x%lx does not match version 0x%lx used by compiler\n", runtime_version, static_version);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "%s (0x%lx)\n", OPENSSL_VERSION_TEXT, runtime_version);
    return EXIT_SUCCESS;
}
