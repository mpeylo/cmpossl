cmake_minimum_required(VERSION 3.7)

project(libcmp)
set(LIBCMP_NAME cmp)

if(NOT DEFINED LIBCMP_VERSION)
  set(LIBCMP_VERSION_MAJOR 2)
  set(LIBCMP_VERSION_MINOR 0)
  set(LIBCMP_VERSION ${LIBCMP_VERSION_MAJOR}.${LIBCMP_VERSION_MINOR})
endif()
message(STATUS "CMPforOpenSSL version ${LIBCMP_VERSION}")

# set(CMAKE_VERBOSE_MAKEFILE ON)

if(APPLE)
  set(CMAKE_INSTALL_PREFIX "tmp")
else()
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
include(GNUInstallDirs) # CMAKE_INSTALL_PREFIX must be set before

# https://cmake.org/cmake/help/v3.6/module/FindOpenSSL.html
if(NOT "$ENV{OPENSSL_DIR}" STREQUAL "")
  set(OPENSSL_ROOT_DIR $ENV{OPENSSL_DIR})
  set(OPENSSL_INCLUDE_DIR "$ENV{OPENSSL_DIR}/include")
  set(OPENSSL_LIB $ENV{OPENSSL_DIR})
endif()
if(DEFINED ENV{OPENSSL_LIB})
  set(OPENSSL_LIB $ENV{OPENSSL_LIB})
endif()
find_package(OpenSSL QUIET REQUIRED COMPONENTS SSL Crypto)
if("$ENV{OPENSSL_DIR}" STREQUAL "" OR NOT DEFINED ENV{OPENSSL_LIB})
  message(STATUS "using OpenSSL package, with version ${OPENSSL_VERSION}")
endif()
message(STATUS "using OpenSSL hdrs from ${OPENSSL_INCLUDE_DIR}")
if(NOT "${OPENSSL_LIB}" STREQUAL "")
  set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_LIB}/libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(OPENSSL_SSL_LIBRARY "${OPENSSL_LIB}/libssl${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(OPENSSL_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
endif()
message(STATUS "using OpenSSL libraries ${OPENSSL_CRYPTO_LIBRARY}, ${OPENSSL_SSL_LIBRARY}")

set(SRC_DIR ${PROJECT_SOURCE_DIR}/crypto)
set(LIBCMP_SRC
  "${PROJECT_SOURCE_DIR}/.github/workflows/build.yml"
  ${SRC_DIR}/crmf/crmf_asn.c
  ${SRC_DIR}/crmf/crmf_err.c
  ${SRC_DIR}/crmf/crmf_lib.c
  ${SRC_DIR}/crmf/crmf_pbm.c
  ${SRC_DIR}/cmp/cmp_asn.c
  ${SRC_DIR}/cmp/cmp_ctx.c
  ${SRC_DIR}/cmp/cmp_err.c
  ${SRC_DIR}/cmp/cmp_http.c
  ${SRC_DIR}/cmp/cmp_hdr.c
  ${SRC_DIR}/cmp/cmp_msg.c
  ${SRC_DIR}/cmp/cmp_protect.c
  ${SRC_DIR}/cmp/cmp_client.c
  ${SRC_DIR}/cmp/cmp_server.c
  ${SRC_DIR}/cmp/cmp_status.c
  ${SRC_DIR}/cmp/cmp_vfy.c
  ${SRC_DIR}/cmp/cmp_util.c
  ${SRC_DIR}/cmp/openssl_backport.c
  ${SRC_DIR}/http/http_client.c
  ${SRC_DIR}/http/http_err.c
  ${SRC_DIR}/http/http_lib.c
)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include/${LIBCMP_NAME}")
set(INC_PUBLIC_HDRS
  ${INC_DIR}/openssl/cmp.h
  ${INC_DIR}/openssl/cmperr.h
  ${INC_DIR}/openssl/cmp_util.h
  ${INC_DIR}/openssl/crmf.h
  ${INC_DIR}/openssl/crmferr.h
  ${INC_DIR}/openssl/cryptoerr_legacy.h
  ${INC_DIR}/openssl/safestack_backport.h
  ${INC_DIR}/openssl/openssl_backport.h
  ${INC_DIR}/openssl/http.h
  ${INC_DIR}/openssl/httperr.h
)

add_library(${LIBCMP_NAME} SHARED ${LIBCMP_SRC})
target_link_libraries(${LIBCMP_NAME} ${OPENSSL_LIBRARIES})

# target_include_directories(${LIBCMP_NAME} PRIVATE ${INC_DIR})
include_directories(SYSTEM ${INC_DIR})
include_directories(SYSTEM ${CMAKE_SYSROOT}/usr/include/cmp)

# must not add the system OpenSSL include dir before ${INC_DIR} etc.
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})

if(DEFINED ENV{NDEBUG} OR NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_BUILD_TYPE Release  # automatically leads to CFLAGS += -DNDEBUG -O3
      CACHE STRING "Choose the type of build." FORCE)
else()
  set(CMAKE_BUILD_TYPE Debug  # automatically leads to CFLAGS += -g  # -O0 not added by default
      CACHE STRING "Choose the type of build." FORCE)
  target_compile_options(${LIBCMP_NAME} PRIVATE -O0)
  set(SANITIZER_FLAGS -fsanitize=address,undefined -fno-sanitize-recover=all)
  target_compile_options(${LIBCMP_NAME} PRIVATE ${SANITIZER_FLAGS})
  target_link_options   (${LIBCMP_NAME} PRIVATE ${SANITIZER_FLAGS})
endif()
message(STATUS "build mode: ${CMAKE_BUILD_TYPE}")

target_compile_options(${LIBCMP_NAME} PRIVATE
  -include ${INC_DIR}/openssl/openssl_backport.h -DCMP_STANDALONE
  -DDEBUG_UNUSED -DPEDANTIC -pedantic
  -Wall -Woverflow -Wextra -Wswitch -Wmissing-prototypes -Wstrict-prototypes
  -Wformat -Wformat-security -Wtype-limits -Wundef -Wconversion
  -Wsign-compare -Wpointer-arith -Wunused-parameter -Wshadow
  -Wno-sign-conversion -Wno-shorten-64-to-32)

set_property(TARGET ${LIBCMP_NAME} PROPERTY C_STANDARD 90)
# target_compile_features(${LIBGENCMP_NAME} PRIVATE c_std_90)  # would have no effect

set_target_properties(${LIBCMP_NAME} PROPERTIES
  VERSION ${LIBCMP_VERSION} SOVERSION ${LIBCMP_VERSION_MAJOR}
  PUBLIC_HEADER "${INC_PUBLIC_HDRS}"
)

if(NOT TARGET build)
  add_custom_target(build COMMAND make all)
endif()


# installation and uninstall

if(DEFINED ENV{ROOTFS})
  set(CMAKE_INSTALL_PREFIX $ENV{ROOTFS} CACHE PATH "comment" FORCE)
endif()

# see also https://gitlab.kitware.com/cmake/cmake/-/issues/24929 and
# https://stackoverflow.com/questions/46490448/how-i-can-add-a-changelog-in-deb-with-cmake
add_custom_command(
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/debian/changelog"
    COMMAND gzip -cn9 "${CMAKE_CURRENT_SOURCE_DIR}/debian/changelog"
    > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
)
if(NOT TARGET changelog)
  add_custom_target(changelog ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
endif()

install(TARGETS ${LIBCMP_NAME}
  LIBRARY
    DESTINATION  "${CMAKE_INSTALL_FULL_LIBDIR}"
    COMPONENT lib
  PUBLIC_HEADER
    DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${LIBCMP_NAME}/openssl"
    COMPONENT dev
)

include(./Pod2Man)
set(pages
  OSSL_CMP_ATAV_set0
  OSSL_CRMF_MSG_set1_regCtrl_regToken
  OSSL_CMP_MSG_get0_header
  OSSL_CMP_CTX_new
  OSSL_CMP_MSG_http_perform
  OSSL_CRMF_MSG_set1_regInfo_certReq
  OSSL_CMP_exec_certreq
  OSSL_CMP_SRV_CTX_new
  OSSL_CRMF_pbmp_new
  OSSL_CMP_HDR_get0_transactionID
  OSSL_CMP_STATUSINFO_new
  OSSL_HTTP_parse_url
  OSSL_CMP_ITAV_new_caCerts
  OSSL_CMP_validate_msg
  OSSL_HTTP_REQ_CTX
  OSSL_CMP_ITAV_set0
  OSSL_CRMF_MSG_get0_tmpl
  OSSL_HTTP_transfer
  OSSL_CMP_log_open
  OSSL_CRMF_MSG_set0_validity
  )
foreach(page ${pages})
  POD2MAN("${CMAKE_CURRENT_SOURCE_DIR}/doc/man3" ${page} 3 "${CMAKE_INSTALL_FULL_MANDIR}" dev)
endforeach()

if(NOT TARGET uninstall)
  add_custom_target(uninstall COMMAND xargs -I{} rm -vf $ENV{DESTDIR}{} <install_manifest.txt)
endif()


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(EXT "deb")
  set(GEN "DEB")
else()
  set(EXT "zip")
  set(GEN "ZIP")
endif()
if(NOT DEFINED CPACK_PACKAGE_NAME)
# generate Debian etc. packages using CPack, relying on installation defined above

SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/mpeylo/cmpossl")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VENDOR "Siemens")
set(CPACK_PACKAGE_CONTACT "David von Oheimb <David.von.Oheimb@siemens.com>")
set(CPACK_PACKAGE_VERSION       ${LIBCMP_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${LIBCMP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBCMP_VERSION_MINOR})
set(CPACK_STRIP_FILES ON)

set(CPACK_COMPONENT_LIB_DESCRIPTION "Certificate Management Prototocl (CMP) library
Adds CMP support to OpenSSL in the form of an extra library")
set(CPACK_COMPONENT_DEV_DESCRIPTION "libcmp C headers and documentation
Development support for standalone CMP library")

#if(APPLE)
#  set(CPACK_GENERATOR "Bundle")
#  set(CPACK_BUNDLE_NAME "${PROJECT_NAME}")
#else()
set(CPACK_GENERATOR ${GEN})

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_LIB_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_DEV_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_LIB_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_DEV_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "libcmp (>= ${CPACK_PACKAGE_VERSION})")
set(CPACK_DEBIAN_DEV_PACKAGE_SUGGESTS "libssl-dev")
# see also https://gitlab.kitware.com/cmake/cmake/-/issues/21834
# CPack Deb does not create postinst when installing to /usr/lib/x86_64-linux-gnu
set(CPACK_DEBIAN_LIB_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/extra/triggers")
set(CPACK_DEBIAN_LIB_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
#endif(APPLE)

set(CPACK_SOURCE_GENERATOR "TGZ")
#set(CPACK_SET_DESTDIR TRUE) # prevents package creation error when using cpack
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_SOURCE_IGNORE_FILES
  ${CPACK_IGNORE_FILES}
  ~$
  /\\.git/
  \\.git$
  \\.deb$
  \\.gz$
  \\.zip$
  \\.so
  \\.dylib$
  /attic/
  /tmp/
  /Makefile$
  CMakeFiles/
  _CPack_Packages/
  \\.cmake$
  /CMakeCache.txt$
  /compile_commands.json$
  /install_manifest.*\\.txt$
  /changelog\.gz$
  /debian/tmp/
  \\.substvars$
  \\.log$
  /debian/\\.debhelper/
  /debian/files$
  /debian/debhelper-build-stamp$
  /debian/${PROJECT_NAME}/
  /debian/${PROJECT_NAME}-dev/
  \\.3$
  \\.3\.gz$
  )

include(CPack)

# see also https://gitlab.kitware.com/cmake/cmake/-/issues/21832
# CPack Deb lacks a way to configure copyright files by component
foreach(component IN LISTS CPACK_COMPONENTS_ALL)
  if(${component} STREQUAL "lib")
    set(suffix "")
  else()
    set(suffix "-${component}")
  endif()
  install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright"
    "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
    DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}${suffix}"
    COMPONENT ${component}
    )
endforeach()

if(NOT TARGET ${EXT})
  set (pack_full_name
       "${CMAKE_CURRENT_BINARY_DIR}/${CPACK_PACKAGE_NAME}-dev_${CPACK_PACKAGE_VERSION}_all.${EXT}")
  add_custom_command(
    OUTPUT "${pack_full_name}"
    DEPENDS build
    COMMENT "Building packages possibly including doxygen run"
    COMMAND cpack # --config CPackConfig.cmake
    COMMAND cpack --config CPackSourceConfig.cmake
    )
  add_custom_target(${EXT} DEPENDS ${pack_full_name})
endif()

endif(NOT DEFINED CPACK_PACKAGE_NAME)
