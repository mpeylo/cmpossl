# CMPforOpenSSL (cmpossl)

This is an intermediate CMP, CRMF, and HTTP version abstraction library
based on OpenSSL. It is needed only if required special CMP features
or fixes are not yet (fully) available in the OpenSSL version being used.

Note that this library offers just a low-level API and does not provide a CLI.
A CMP CLI is provided both by [OpenSSL](https://github.com/openssl/openssl)
and by the [Generic CMP Client](https://github.com/siemens/gencmpclient).\
The genCMPClient offers a more high-level API.
It can be built using OpenSSL 3.0 or later (and possibly this library,
which was formerly needed in order to provide more recent developments).

## Purpose

The purpose of this software is to provide a uniform interim CMP and HTTP client
API and implementation library that links with all current OpenSSL versions.

Since version 3.0, [OpenSSL](https://openssl-library.org/) includes
an implementation of CMP version 2 and CRMF, as well as a lean HTTP client.
As of November 2024,
upstream contribution of the features of CMP version 3 according to the
[Lightweight CMP Profile (LCMPP)](https://www.rfc-editor.org/rfc/rfc9483)
to OpenSSL is nearly finished. OpenSSL version 3.4 contains all of them except
for [central key generation](https://github.com/openssl/openssl/pull/25132).
In version 3.5, to be released in April 2025, this integration will be complete.
Therefore, in most cases this intermediate library meanwhile is not needed anymore.
<!--
Software that is based on earlier OpenSSL versions can make use of this library
in order to use CMP and/or the HTTP client capabilities also with OpenSSL 1.x.
-->

## Support model

As far as still required, the [maintainers](MAINTAINERS)
offer paid professional support upon request.

<!-- two levels of support.
* Community support is provided on a best-effort basis
  and can be requested via [issues](../../issues).
* Paid professional support and consulting can be ordered
  from Siemens by reaching out to the maintainers.

Contributions can be provided in the form of [pull requests](../../pulls).
-->

## Further information

Unmaintained further information may be found in the [former README file](README_old.md).


## Disclaimer

This software including associated documentation is provided ‘as is’.
Effort has been spent on quality assurance, but there are no guarantees.


## License

This work is licensed under the terms of the Apache Software License 2.0.
See the [LICENSE.txt](LICENSE.txt) file.

SPDX-License-Identifier: Apache-2.0
