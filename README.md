# CMPforOpenSSL (cmpossl)

This is a CMP and HTTP abstraction layer library based on OpenSSL.
As opposed to the [Generic CMP Client](https://github.com/siemens/gencmpclient), this does not provide a CLI.


## Purpose

The purpose of this software is to provide a uniform interim CMP and HTTP client
API as a standalone library that links with all current OpenSSL versions.
The library aims at supporting all features of CMP version 3.
The [generic CMP client](https://github.com/siemens/gencmpclient) is based on it.

Since version 3.0, [OpenSSL](https://www.openssl.org/) includes
an implementation of CMP version 2 and CRMF, as well as a lean HTTP client.
Software that is based on earlier OpenSSL versions can make use of this library
in order to use CMP and/or the HTTP client capabilities also with OpenSSL 1.x.
<!--
Yet also software based on OpenSSL 3.0+ can benefit from using this library
because it provides uniform access to this functionaliy
as well as enhancements not (yet) included in the latest OpenSSL release.
-->


## Status and changelog

As of late 2021, this CMP version is being standardized at the IETF, see
[Certificate Management Protocol (CMP) Updates](https://datatracker.ietf.org/doc/html/draft-ietf-lamps-cmp-updates)
and is being geared towards simple and interoperable industrial use by the
[Lightweight Certificate Management Protocol (CMP) Profile](https://datatracker.ietf.org/doc/html/draft-ietf-lamps-lightweight-cmp-profile).

Currently the new feature defined there are being implemented.


## Documentation

API documentation is available in the [`doc/man3`](doc/man3) folder.


## Prerequisites

This software should work with any flavor of Linux, including [Cygwin](https://www.cygwin.com/),
also on a virtual machine or the Windows Subsystem for Linux ([WSL](https://docs.microsoft.com/windows/wsl/about)).

The following network and development tools are needed or recommended.
* Git (for getting the software, tested with versions 2.7.2, 2.11.0, 2.20, 2.30.2)
* GNU make (tested with versions 4.1, 4.2.1, 4.3)
* GNU C compiler (gcc, tested with versions 5.4.0, 7.3.0, 8.3.0, 10.0.1, 10.2.1)

The following OSS components are used.
* OpenSSL development edition (tested with versions 1.0.2, 1.1.0, 1.1.1, 3.0)

For instance, on a Debian system these may be installed simply as follows:
```
sudo apt install libssl-dev
```
while `apt install wget git make gcc` usually is not needed as far as these tools are pre-installed.

As a sanity check you can execute in a shell:
```
git clone git@github.com:mpeylo/cmpossl.git --depth 1
cd cmpossl
make -f OpenSSL_version.mk
```
In order for this to work, you may need to set OPENSSL_DIR as described below,
e.g.,
```
export OPENSSL_DIR=/usr/local
```

This should output on the console something like
```
cc [...] OpenSSL_version.c -lcrypto -o OpenSSL_version
OpenSSL 1.1.1n  15 Mar 2022 (0x101010ef) == runtime version 0x101010ef
rm -f OpenSSL_version
```


## Getting the software

For accessing the code repositories on GitHub you may need
an SSH client with suitable credentials or an HTTP proxy set up, for instance:
```
export https_proxy=http://proxy.my-company.com:8080
```

You can clone the git repository with
```
git clone git@github.com:mpeylo/cmpossl.git --depth 1
```

For using the project as a git submodule,
do for instance the following in the directory where you want to integrate it:
```
git submodule add git@github.com:mpeylo/cmpossl.git
```

When you later want to update your local copy of all relevant repositories it is sufficient to invoke
```
make update
```


## Building the software

The library assumes that OpenSSL (with any version >= 1.1.0) is already installed,
including the C header files needed for development (as provided by, e.g., the Debian/Ubuntu package `libssl-dev`).
By default the OpenSSL headers will be searched for in `/usr/include` and its shared objects in `/usr/lib` (or `/usr/bin` for Cygwin).
You may point the environment variable `OPENSSL_DIR` to an alternative OpenSSL installation, e.g.:
```
export OPENSSL_DIR=/usr/local
```
You may also specify using the environment variable `OUT_DIR`
where the produced library (e.g., `libcmp.so.1`) shall be placed.
By default, the current directory (`.`) is used.
For further details on optional environment variables,
see the [`Makefile`](Makefile).

In the newly created directory `cmpossl` you can build the software simply with
```
make
```
where the CC environment variable may be set as needed; it defaults to %'gcc'.
<!--
Also the ROOTFS environment variable may be set, e.g., for cross compilation.
-->

The result is in, for instance, `./libcmp.so.1`.


## Using the library in own applications

For compiling applications using the library,
you will need to add the directory [`include/cmp`](include/cmp/)
to your C headers path.

For linking you will need to refer the linker to the library, e.g., `-lcmp`
and add the directory (e.g., with the linker option `-L`) where it can be found.
See also the environment variable `OUT_DIR`.
For helping the Linux loader to find the libraries at run time,
it is recommended to set also linker options like `-Wl,-rpath=.`.

Also make sure that the OpenSSL libraries (typically referred to via `-lssl -lcrypto`) are in your library path and
(the version) of the libraries found there by the linker match the header files found by the compiler.


## Building Debian packages

This repository can build two Debian packages.

* `libcmp` - the shared library
* `libcmp-dev` - development headers

To build the Debian packages, the following dependencies need to be installed:
* `debhelper`
* `libssl-dev`

Then the packages can be built by
```
make deb
```
On success, they are placed in the parent directory (`../`).

## Disclaimer

This software including associated documentation is provided ‘as is’.
Effort has been spent on quality assurance, but there are no guarantees.


## License

This work is licensed under the terms of the Apache Software License 2.0.
See the [LICENSE.txt](LICENSE.txt) file in the top-level directory.

SPDX-License-Identifier: Apache-2.0
