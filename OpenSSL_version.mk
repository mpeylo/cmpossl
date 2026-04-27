#!/usr/bin/make
#
# keep this file identical among genCMPClient and CMPforOpenSSL
#
# target can be 'dir' (yielding OPENSSL_DIR), 'lib' (yielding OPENSSL_LIB),
#   or by default the OpenSSL version will be detected. In this case:
#   SOURCE can be 'app', 'header', a library file path name, or unset
#   if 'app' or unset, leads to default behavior: build and run OpenSSL_version
# Optional OPENSSL_DIR defines where to find the OpenSSL installation
#   with header files at include/openssl (default: will try, e.g., /usr).
# Optional OPENSSL_LIB defines where to find the OpenSSL libraries
#   (default: will try, e.g., OPENSSL_DIR/lib).

# variables ####################################################################

# https://stackoverflow.com/questions/714100/os-detecting-makefile
ifeq ($(OS),Windows_NT) # strange but apparently this string is used also for all later versions
    # so far, we do not support Windows, but trying to continue anyway
    override OS=Windows
    USERS='^([[:alpha:]]:)?\\Users\\'
    $(warning [DEBUG] OS is $(OS))
    DLL = .dll
    LDD=TODO
    EXE = .exe
    LIB=bin
else
    override OS = $(shell sh -c 'uname 2>/dev/null || echo Unknown')
    USERS='^/(home|Users)/'
    ifeq ($(shell uname -s),Darwin)
        override OS = MacOS
        DLL = .dylib
        LDD=otool -l
        $(warning [DEBUG] OS is Darwin ($(OS)))
    else
        $(warning [DEBUG] OS is supposed to be Unix-like: $(OS))
        DLL = .so
        LDD=ldd
    endif
    LIB=lib
    EXE =
endif

SOURCE ?= app


# determine OPENSSL_DIR and OPENSSL_LIB ########################################

ifeq ($(filter-out app header,$(SOURCE))$(filter-out lib dir,$(MAKECMDGOALS)),)
# OPENSSL_DIR and OPENSSL_LIB not needed for detecting version from library file

ifeq ($(OPENSSL_DIR),) # for convenience, use heuristics to determine OPENSSL_DIR
    ifeq ($(OS),MacOS)
        SYSTEM_INCLUDE_OPENSSL=/opt/homebrew/include/openssl
    else
        SYSTEM_INCLUDE_OPENSSL=/usr/include/openssl
    endif
    OPENSSL_INCLUDE_DIR = $(realpath $(SYSTEM_INCLUDE_OPENSSL))
    override OPENSSL_DIR = $(realpath $(OPENSSL_INCLUDE_DIR)/../..)
    $(warning [DEBUG] OPENSSL_DIR is detected as $(OPENSSL_DIR))
else
    $(warning [DEBUG] OPENSSL_DIR is set to $(OPENSSL_DIR))
endif

ifeq ($(OS),Windows)
    LIB = bin
else
    LIB = lib
endif

ifeq ($(OPENSSL_DIR),) # due to the above, always false
    $(warning Warning: OPENSSL_DIR is empty or not set, and was unable to determine it, trying to continue anyway)
else
    LIB_NAME_PATTERN=libcrypto*$(DLL)*
    OPENSSL_FULL_DIR = $(realpath $(OPENSSL_DIR))
    ifneq ($(OPENSSL_FULL_DIR),$(OPENSSL_DIR))
        $(warning [DEBUG] OPENSSL_DIR expands to $(OPENSSL_FULL_DIR))
    endif
    ifeq ($(OPENSSL_FULL_DIR),)
        $(error OPENSSL_DIR appears to be an invalid path: $(OPENSSL_DIR))
    endif
    ifeq ($(OPENSSL_LIB),) # for convenience, use heuristics to determine OPENSSL_LIB
        override OPENSSL_LIB = $(OPENSSL_DIR)/$(LIB)
        ifeq ($(wildcard $(OPENSSL_LIB)/$(LIB_NAME_PATTERN)),)
            $(warning Warning: cannot find OpenSSL libraries at determined default location $(OPENSSL_LIB), now trying $(OPENSSL_DIR))
            override OPENSSL_LIB = $(OPENSSL_DIR)
            ifeq ($(wildcard $(OPENSSL_LIB)/$(LIB_NAME_PATTERN)),)
                ifeq ($(OS),Linux)
	            ifeq ($(shell echo $(OPENSSL_FULL_DIR) | grep -E $(USERS)),)
                        override OPENSSL_LIB = $(wildcard /lib/$(shell uname -i)-linux-*)
                        $(warning Warning: cannot find OpenSSL libraries at $(OPENSSL_DIR), now trying $(OPENSSL_LIB))
                    endif
                endif
            endif
        endif
    else
        ifeq ($(wildcard $(OPENSSL_LIB)/$(LIB_NAME_PATTERN)),)
            $(warning Warning: cannot find OpenSSL libraries at given OPENSSL_LIB $(OPENSSL_LIB), now trying OPENSSL_DIR)
            override OPENSSL_LIB = $(OPENSSL_DIR)
        endif
    endif
    # ifeq ($(findstring $(USERS),$(OPENSSL_FULL_DIR)),)
    #     $(warning [DEBUG] OPENSSL_DIR is assumed to be an installation directory)
    # else
    #     $(warning [DEBUG] OPENSSL_DIR is assumed to be a local build directory)
    # endif
    ifeq ($(wildcard $(OPENSSL_LIB)/$(LIB_NAME_PATTERN)),)
        $(warning Warning: cannot find OpenSSL library $(LIB_NAME_PATTERN) at $(OPENSSL_LIB)/, trying to continue anyway)
    endif
endif
endif # eq ($(filter-out app header,$(SOURCE))$(filter-out lib dir,$(MAKECMDGOALS)),)


# detect version ###############################################################

# https://stackoverflow.com/questions/17055773/how-to-synthesize-line-breaks-in-gnu-make-warnings-or-errors
define newline # the following two empty lines are essential


endef

ifneq ($(MAKECMDGOALS),dir)
ifneq ($(MAKECMDGOALS),lib)
$(warning [DEBUG] parameter SOURCE=$(SOURCE))
ifeq ($(SOURCE),app)
    $(warning [DEBUG] SOURCE is app or unset (default))

    LDFLAGS += -L$(OPENSSL_LIB) -Wl,-rpath,$(OPENSSL_LIB)
    CFLAGS += -isystem $(OPENSSL_DIR)/include
    LDLIBS = -lcrypto

  # $(warning [TRACE] EXE=$(EXE))
    $(warning [TRACE] OPENSSL_DIR=$(OPENSSL_DIR))
    $(warning [TRACE] OPENSSL_LIB=$(OPENSSL_LIB))
    $(warning [TRACE] CFLAGS=$(CFLAGS))
    $(warning [TRACE] LDLIBS=$(LDLIBS))
    $(warning [TRACE] LDFLAGS=$(LDFLAGS)$(newline))

    .PHONY: default build show clean

    default: clean build show

    build: OpenSSL_version

    show: build fix_build_lib
	@./OpenSSL_version$(EXE)

    clean:
	@rm -f OpenSSL_version$(EXE)

else ifeq ($(SOURCE),header)
    # $(warning [DEBUG] SOURCE is 'header')
    OPENSSLV_H = $(OPENSSL_DIR)/include/openssl/opensslv.h
    ifeq ($(OPENSSL_DIR),)
        $(error missing OPENSSL_DIR, which is needed for reading OPENSSL_DIR$(OPENSSLV_H))
    endif
    $(warning [DEBUG] OPENSSLV_H=$(OPENSSLV_H))
    ifeq ($(shell fgrep OPENSSL_VERSION_MAJOR "$(OPENSSLV_H)"),)
        $(warning [DEBUG] OPENSSL_VERSION_MAJOR not found in OPENSSLV_H)
        OPENSSL_VERSION = $(shell grep 'OPENSSL_VERSION_TEXT\s* "OpenSSL ' "$(OPENSSLV_H)" \
                            | head -n 1 | sed -r 's/.*OpenSSL //' | awk '{print ($$0+0)}')
    else
        $(warning [DEBUG] OPENSSL_VERSION_MAJOR found in OPENSSLV_H)
        OPENSSL_VERSION = $(shell fgrep 'define OPENSSL_VERSION_M' "$(OPENSSLV_H)" \
                            | awk 'BEGIN { RS="" } {print $$4"."$$8 }')
        # $(warning [TRACE] After splitting OPENSSL_VERSION_MAJOR/MINOR: OPENSSL_VERSION=$(OPENSSL_VERSION))
    endif

    ifeq ($(OPENSSL_VERSION),1)
        OPENSSL_VERSION = 1.0
    endif
    $(warning [TRACE] OPENSSL_VERSION=$(OPENSSL_VERSION))

else # $(SOURCE) is name of library file (libcrypto or libsecutils or libcmp)
    $(warning [DEBUG] SOURCE is supposed to be a library file: $(SOURCE))
    ifeq ($(realpath $(SOURCE)),)
        $(error SOURCE appears to be an invalid file path name: $(SOURCE))
    endif

    # first try to find version string in library binary because this is more accurate than the file name
    OPENSSL_VERSION = $(shell strings "$(SOURCE)" | grep -E 'OpenSSL [0-9]+\.[0-9]+\.' \
                        | head -n 1 | sed -r 's/.*OpenSSL //' | awk -v FS="." '{print $$1"."$$2}')
    ifeq ($(OPENSSL_VERSION),)
        $(warning [DEBUG] OpenSSL version info not found in library file contents; now trying to get it from the libcrypto file name referenced)
        ifeq ($(findstring libcrypto,$(SOURCE)),)
            STR=$(shell $(LDD) "$(SOURCE)" | grep -E 'libcrypto\.' | head -n 1)
            ifeq ($(STR),)
                $(error Error using '$(LDD)' to get the library dependencies in $(SOURCE))
            endif
        else
            STR="$(SOURCE)"
        endif
#       OPENSSL_VERSION = $(shell echo "'$(STR)'" | sed -r 's/.*libcrypto(\.\w+)?\.(\d+(\.d+)?)/$1/')
        OPENSSL_VERSION = $(shell echo "'$(STR)'" | sed -r 's/.*libcrypto(\.[[:alpha:]]+)?\.([[:digit:]](\.[[:digit:]]+)?).*/\2/')
    endif
    ifeq ($(OPENSSL_VERSION),1.0.0)
        OPENSSL_VERSION = 1.0
    endif

    $(warning [TRACE] OPENSSL_VERSION=$(OPENSSL_VERSION))

endif # $(SOURCE)
endif #neq ($(MAKECMDGOALS),lib)
endif #neq ($(MAKECMDGOALS),dir)


# report result ################################################################

.PHONY: detect # must be the first rule in this sequence to make it the default
detect:
ifneq ($(SOURCE),app)
    ifeq ($(shell echo "$(OPENSSL_VERSION)" | grep -E '^[[:digit:]]+(\.[[:digit:]]+)*$$'),)
        $(error Error detecting OpenSSL version from $(SOURCE))
    endif
endif
	@echo $(OPENSSL_VERSION)

.PHONY: dir
dir:
ifeq ($(MAKECMDGOALS),dir)
    ifeq ($(realpath $(OPENSSL_DIR)),)
        $(error Error determining OPENSSL_DIR)
    endif
endif
	@echo "$(realpath $(OPENSSL_DIR))"

.PHONY: lib
lib: fix_build_lib
ifeq ($(MAKECMDGOALS),dir)
    ifeq ($(realpath $(OPENSSL_LIB)),)
        $(error Error determining OPENSSL_LIB)
    endif
endif
	@echo "$(realpath $(OPENSSL_LIB))"


# workaround for using local OpenSSL builds by default expecting that
# its dynamic libs have been installed in ./$(LIB) when using the libs
# see for binaries that dynamically link to OpenSSL the output of $(LDD) <binary>
.PHONY: fix_build_lib
fix_build_lib:
ifneq ($(shell echo $(realpath $(OPENSSL_LIB)) | grep -E $(USERS)),)
    ifeq ($(OPENSSL_LIB),$(OPENSSL_DIR))
	@cd "$(OPENSSL_DIR)"; if [ ! -e $(LIB) ]; then ln -s . $(LIB); fi
	@ # alternative would be to use, e.g.,
	@ # install_name_tool -change $(OPENSSL_DIR)/lib/libcrypto.3.dylib $(OPENSSL_DIR)/libcrypto.3.dylib <libname>
    endif
endif
	@true # prevent warning "Nothing to be done for `fix_build_lib'."
