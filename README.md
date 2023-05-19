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
* CMake (for using [`CMakeLists.txt`](CMakeLists.txt), tested with versions 3.18.4 and 3.26.3)
* GNU make (tested with versions 4.1, 4.2.1, 4.3)
* GNU C compiler (gcc, tested with versions 5.4.0, 7.3.0, 8.3.0, 10.0.1, 10.2.1)

The following OSS components are used.
* OpenSSL development edition (tested with versions 1.0.2, 1.1.0, 1.1.1, 3.0)

For instance, on a Debian system the prerequisites may be installed simply as follows:
```
sudo apt install libssl-dev libc-dev linux-libc-dev
```
while `apt install git make gcc` usually is not needed as far as these tools are pre-installed.

As a sanity check you can execute in a shell:
```
git clone git@github.com:mpeylo/cmpossl.git --depth 1
cd cmpossl
make -f OpenSSL_version.mk
```
This should output on the console something like
```
cc [...] OpenSSL_version.c -lcrypto -o OpenSSL_version
OpenSSL 3.0.8 7 Feb 2023 (0x30000080)
```

You might need to set the variable `OPENSSL_DIR` first as described below, e.g.,
```
export OPENSSL_DIR=/usr/local
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


## Configuring and building

The library assumes that OpenSSL is already installed,
including the C header files needed for development
(as provided by, e.g., the Debian/Ubuntu package `libssl-dev`).

By default any OpenSSL installation available on the system is used.
Set the optional environment variable `OPENSSL_DIR` to specify the
absolute (or relative to `../`) path of the OpenSSL installation to use, e.g.:
```
export OPENSSL_DIR=/usr/local
```
In case its libraries are in a different location, set also `OPENSSL_LIB`, e.g.:
```
export OPENSSL_LIB=$OPENSSL_DIR/lib
```

Since version 2, it is recommended to use CMake to produce the `Makefile`,
for instance as follows:
```
cmake .
```

For backward compatibility it is also possible to use instead of CMake the
pre-defined [`Makefile_v1`](Makefile_v1); to this end symlink it to `Makefile`:
```
ln -s Makefile_v1 Makefile
```
or use for instance `make -f Makefile_v1`.

In this case you may also specify using the environment variable `OUT_DIR`
where the produced library files (e.g., `libcmp.so.2.0`) shall be placed.
By default, the current directory (`.`) is used.\
The environment variable `CC` may be set as needed; it defaults to `gcc`.\
For further details on optional environment variables,
see the [`Makefile_v1`](Makefile_v1).

Build the software with
```
make
```

By default, builds are done in Debug mode.
For Release mode use `-DCMAKE_BUILD_TYPE=Release` or `NDEBUG=1`.
For switching to Debug mode, use `-DCMAKE_BUILD_TYPE=Debug` and unset `NDEBUG`.

The result is in, for instance, `./libcmp.so.2.0`.

When using CMake, `cmake` must be (re-)run
after setting or unsetting environment variables.


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


### Installing and uninstalling

The software can be installed with, e.g.,
```
sudo make install
```
and uninstalled with
```
sudo make uninstall
```

The destination is `/usr`, unless specified otherwise by `DESTDIR` or `ROOTFS`.


## Building Debian packages

This repository can build the following Debian and source packages.

* `libcmp` -- the shared library
* `libcmp-dev` -- development headers and documentation
* `libcmp*Source.tar.gz` -- source tarball

The recommended way is to use CPack with files produced by CMake,
for instance as follows:
```
make deb
```

The recommended way is to use CPack with the files produced by CMake as follows:
```
make deb
```

Alternatively, [`Makefile_v1`](Makefile_v1) may be used like this:
```
make -f Makefile_v1 deb
```
In this case, the resulting packages are placed in the parent directory (`../`),
and the following dependencies need to be installed:
* `debhelper` (needed for `dh`)
* `devscripts` (needed for `debuild`)
* `libssl-dev`

The Debian packages may be installed for instance as follows:
```
sudo dpkg -i libcmp*deb
```


## Disclaimer

This software including associated documentation is provided ‘as is’.
Effort has been spent on quality assurance, but there are no guarantees.


## License

This work is licensed under the terms of the Apache Software License 2.0.
See the [LICENSE.txt](LICENSE.txt) file in the top-level directory.

SPDX-License-Identifier: Apache-2.0
