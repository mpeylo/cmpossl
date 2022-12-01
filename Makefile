#!/bin/make
# Optional OUT_DIR defines where the resulting cmp lib will be placed (default: ".").
# Optional LIBCMP_INC defines where the libcmp header files will be placed (default: "OUT_DIR/include_cmp").
# Optional OPENSSL_DIR defines where to find the OpenSSL installation (default: "/usr" or ".").
# All these paths may be absolute or relative to the dir containing this Makefile.
# Optional DEBUG_FLAGS may set to prepend to local CFLAGS and LDFLAGS (default see below).

SHELL=bash # This is needed for supporting extended file name globbing

ifeq ($(OS),Windows_NT)
#   EXE=.exe
    DLL=.dll
    OBJ=.obj
    LIB=bin
else
#   EXE=
    DLL=.so
    OBJ=.o
    LIB=lib
endif

ROOTFS ?= $(DESTDIR)$(prefix)

VERSION=1.0
# must be kept in sync with latest version in debian/changelog
# PACKAGENAME=libcmp
# DIRNAME=$(PACKAGENAME)-$(VERSION)

ifeq ($(OUT_DIR),)
     OUT_DIR=.
endif

SYSTEM_INCLUDE_OPENSSL=/usr/include/openssl
ifeq ($(OPENSSL_DIR),)
   ifneq (,$(wildcard $(SYSTEM_INCLUDE_OPENSSL)))
     OPENSSL_DIR=/usr
   else
     OPENSSL_DIR=.
   endif
endif
ifeq ($(shell echo $(OPENSSL_DIR) | grep "^/"),)
# $(OPENSSL_DIR) is relative path
    OPENSSL=$(OPENSSL_DIR)
    OPENSSL_LIB=$(OPENSSL)
    OPENSSL_RPATH=$(OPENSSL)
    OPENSSL_RPATH_LIB=$(OPENSSL)
else
# $(OPENSSL_DIR) is absolute path
    OPENSSL=$(OPENSSL_DIR)
    OPENSSL_LIB=$(OPENSSL)/$(LIB)
    OPENSSL_RPATH=$(OPENSSL_DIR)
    OPENSSL_RPATH_LIB=$(OPENSSL_LIB)
endif

MAKECMDGOALS ?= default
ifneq ($(filter-out doc update install uninstall clean clean_install clean_deb,$(MAKECMDGOALS)),)
OPENSSL_VERSION=$(shell $(MAKE) -s --no-print-directory -f OpenSSL_version.mk LIB=h OPENSSL_DIR="$(OPENSSL_DIR)")
ifeq ($(OPENSSL_VERSION),)
    $(warning cannot determine version of OpenSSL in directory '$(OPENSSL_DIR)', assuming 1.1.1)
    OPENSSL_VERSION=1.1.1
endif
$(info detected OpenSSL version $(OPENSSL_VERSION).x)
OSSL_VERSION_QUIRKS=-D'DEPRECATEDIN_1_2_0(f)= ' # needed for 1.2
ifeq ($(shell expr "$(OPENSSL_VERSION)" \< 1.1),1) # same as comparing == 1.0
    $(info enabling compilation quirks for OpenSSL 1.0.2)
    OSSL_VERSION_QUIRKS+=-Wno-discarded-qualifiers -Wno-unused-parameter #-Wno-unused-function #-D'DEPRECATEDIN_1_1_0(f)=f;' -D'DEPRECATEDIN_1_0_0(f)='
endif
endif

.phony: default
default: build

.phony: update
update:
	git fetch
	git rebase origin

LIBCMP_INC ?= $(OUT_DIR)/include_cmp
OUTLIB=libcmp$(DLL)

CC ?= gcc
ifdef NDEBUG
    DEBUG_FLAGS ?= -O2
    override DEBUG_FLAGS += -DNDEBUG=1
else
    DEBUG_FLAGS ?= -g -O0 -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all # not every compiler(version) supports -Og
endif
override CFLAGS += $(DEBUG_FLAGS) -fPIC -DDEBUG_UNUSED -DPEDANTIC -pedantic -Wall -Wextra -Wswitch -Wsign-compare -Wmissing-prototypes -Wstrict-prototypes -Wshadow -Wformat -Wtype-limits -Wundef $(OSSL_VERSION_QUIRKS)# -Werror # not needed for cmp+crmf: -Wno-long-long -Wno-missing-field-initializers
override CFLAGS += -isystem $(OPENSSL_DIR)/include # use of -isystem is critical for selecting wanted OpenSSL version
override CFLAGS += -I$(LIBCMP_INC)
#LIBCMP_HDRS_INC = -include $(LIBCMP_INC)/openssl/crmf.h # used to force inclusion of standalone version, needed also for cmp_err.c
LIBCMP_HDRS_INC = -include $(LIBCMP_INC)/openssl/openssl_backport.h # used to force inclusion of standalone version

override LDFLAGS += -L$(OPENSSL_LIB) -L$(OPENSSL)
ifeq ($(DEB_TARGET_ARCH),) # not during Debian packaging
    override LDFLAGS += -Wl,-rpath=$(OPENSSL_RPATH_LIB) -Wl,-rpath=$(OPENSSL_RPATH)
endif
override LDLIBS  += -lcrypto

LIBCMP_HDRS_= cmp_util.h cmp.h cmperr.h crmf.h crmferr.h http.h httperr.h \
  safestack_backport.h openssl_backport.h cryptoerr_legacy.h
LIBCMP_HDRS_INTERNAL_ = sizes.h constant_time.h cryptlib.h sockets.h common.h nelem.h e_os.h crmf.h
LIBCMP_INC_HDRS          = $(patsubst %,$(LIBCMP_INC)/openssl/%,$(LIBCMP_HDRS_))
LIBCMP_INC_HDRS_INTERNAL = $(patsubst %,$(LIBCMP_INC)/internal/%,$(LIBCMP_HDRS_INTERNAL_))
CMP_SRCS_ = cmp_asn.c cmp_ctx.c cmp_err.c cmp_http.c cmp_hdr.c cmp_msg.c cmp_protect.c cmp_client.c cmp_server.c cmp_status.c cmp_vfy.c cmp_util.c openssl_backport.c
CRMF_SRCS_ = crmf_asn.c crmf_err.c crmf_lib.c crmf_pbm.c
HTTP_SRCS_ = http_client.c http_err.c http_lib.c
LIBCMP_SRCS = $(patsubst %,crypto/crmf/%,$(CRMF_SRCS_)) \
    $(patsubst %,crypto/cmp/%,$(CMP_SRCS_)) \
    $(patsubst %,crypto/http/%,$(HTTP_SRCS_))

.phony: build clean

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)
build: $(OUT_DIR) $(OUT_DIR)/$(OUTLIB)

$(LIBCMP_INC)/openssl:
	@mkdir -p $(LIBCMP_INC)/openssl
ifeq ($(shell expr "$(OPENSSL_VERSION)" \< 3.0),1)
	@cd $(LIBCMP_INC)/openssl; touch macros.h types.h trace.h
endif

$(LIBCMP_INC)/internal:
	@mkdir -p $(LIBCMP_INC)/internal

$(LIBCMP_INC)/openssl/%.h: include/openssl/%.h | $(LIBCMP_INC)/openssl
	@cp $< $@ # --preserve=timestamps has no effect on WSL   

$(LIBCMP_INC)/internal/%.h: include/internal/%.h | $(LIBCMP_INC)/internal
	@cp $< $@ # --preserve=timestamps has no effect on WSL   

$(OUT_DIR)/$(OUTLIB).$(VERSION): $(LIBCMP_INC_HDRS) $(LIBCMP_INC_HDRS_INTERNAL) $(LIBCMP_SRCS)
	@ # cd $(LIBCMP_INC)/openssl && ((mv crmf.h tmp2.h && /bin/echo -e "#undef CMP_STANDALONE\n#define CMP_STANDALONE\n" >tmp1.h && cat tmp1.h tmp2.h >crmf.h && touch -r tmp2.h crmf.h); rm -f tmp1.h tmp2.h)
	$(CC) -DCMP_STANDALONE $(CFLAGS) $(LIBCMP_HDRS_INC) $(LIBCMP_SRCS) $(LDFLAGS) $(LDLIBS) -shared -o $@ -Wl,-soname,$(OUTLIB).$(VERSION)

$(OUT_DIR)/$(OUTLIB): $(OUT_DIR)/$(OUTLIB).$(VERSION)
	ln -sf $(OUTLIB).$(VERSION) $(OUT_DIR)/$(OUTLIB)

clean: clean_deb
	rm -f $(LIBCMP_INC)/openssl/* $(LIBCMP_INC)/internal/*
	rmdir $(LIBCMP_INC)/openssl $(LIBCMP_INC)/internal || true
	rm -f $(OUT_DIR)/$(OUTLIB)*
	rmdir $(LIBCMP_INC)/openssl $(LIBCMP_INC)/internal $(LIBCMP_INC) 2>/dev/null || true

SYSTEM_LIB=/usr/lib
DEST_LIB=$(ROOTFS)$(SYSTEM_LIB)
DEST_INC=$(ROOTFS)$(SYSTEM_INCLUDE_OPENSSL)
DEST_DOC=$(ROOTFS)/usr/share/doc/libcmp-dev# TODO improve
LIBCMP_HDRS_install = $(patsubst %,$(DEST_INC)/%,$(LIBCMP_HDRS_))
LIBCMP_DOCS_ = $(wildcard doc/man3/*.pod)
LIBCMP_DOCS_install = $(patsubst doc/man3/%,$(DEST_DOC)/%,$(LIBCMP_DOCS_))

.phony: install uninstall clean_install

install: # $(OUT_DIR)/$(OUTLIB).$(VERSION)
	install -D $(OUT_DIR)/$(OUTLIB).$(VERSION) $(DEST_LIB)/$(OUTLIB).$(VERSION)
	ln -sf $(OUTLIB).$(VERSION) $(DEST_LIB)/$(OUTLIB)
#install_headers:
	mkdir -p $(DEST_INC)
	install -D $(LIBCMP_INC)/openssl/*.h $(DEST_INC)
#install_doc:
	mkdir -p $(DEST_DOC)
	install -D $(LIBCMP_DOCS_) $(DEST_DOC)

clean_install:
	rm -f $(DEST_LIB)/$(OUTLIB)*
	rm -f $(LIBCMP_HDRS_install)
	rm -f $(LIBCMP_DOCS_install)

uninstall: clean_install

#SRCS=$(shell ls Makefile_cmp include/openssl/{{cmp,crmf,http}{,err}.h,safestack_backport.h} crypto/{cmp,crmf,http}/*.{c,h})
#SRCS_TAR=cmpossl_0.1.0.orig.tar.gz
.phony: deb clean_deb
deb:
	@ #tar czf $(SRCS_TAR) $(SRCS)
	@ #rm -f $(OUTLIB).$(VERSION) debian/tmp/usr/lib/libcmp.so*
	debuild -uc -us --lintian-opts --profile debian # --fail-on none
	@ #rm $(SRCS_TAR)
# alternative:
#	LD_LIBRARY_PATH= dpkg-buildpackage -uc -us # may prepend DH_VERBOSE=1

clean_deb:
	rm -rf debian/tmp debian/libcmp{,-dev}
	rm -f debian/{files,debhelper-build-stamp} debian/*.{log,substvars}
	rm -f ../libcmp{_,-}*
