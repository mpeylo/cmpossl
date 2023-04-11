/*
 * Copyright 1995-2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_E_OS_H
# define OSSL_E_OS_H

/* system-specific variants defining ossl_sleep() */
#  ifdef OPENSSL_SYS_UNIX
#   include <unistd.h>
static ossl_inline void ossl_sleep(unsigned long millis)
{
#   ifdef OPENSSL_SYS_VXWORKS
    struct timespec ts;
    ts.tv_sec = (long int) (millis / 1000);
    ts.tv_nsec = (long int) (millis % 1000) * 1000000ul;
    nanosleep(&ts, NULL);
#   elif defined(__TANDEM) && !defined(_REENTRANT)
#    include <cextdecs.h(PROCESS_DELAY_)>
    /* HPNS does not support usleep for non threaded apps */
    PROCESS_DELAY_(millis * 1000);
#   else
    usleep((unsigned int)(millis * 1000));
#   endif
}
#  elif defined(_WIN32)
#   include <windows.h>
static ossl_inline void ossl_sleep(unsigned long millis)
{
    Sleep(millis);
}
#  else
/* Fallback to a busy wait */
#   include <sys/time.h>
static ossl_inline void ossl_sleep(unsigned long millis)
{
    struct timeval start, now;
    unsigned long elapsedms;

    gettimeofday(&start, NULL);
    do {
        gettimeofday(&now, NULL);
        elapsedms = (((now.tv_sec - start.tv_sec) * 1000000)
                     + now.tv_usec - start.tv_usec) / 1000;
    } while (elapsedms < millis);
}
#  endif /* defined OPENSSL_SYS_UNIX */

#endif
