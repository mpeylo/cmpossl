cmake_minimum_required(VERSION 3.7)

project(cmp)
include(GNUInstallDirs)

if(NOT DEFINED CMPOSSL_VERSION)
  set(CMPOSSL_VERSION_MAJOR 2)
  set(CMPOSSL_VERSION ${CMPOSSL_VERSION_MAJOR}.0)
endif()

message(STATUS "CMPforOpenSSL version " ${CMPOSSL_VERSION})

# set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT("$ENV{OPENSSL_DIR}" STREQUAL ""))
  message(STATUS "using OpenSSL from " $ENV{OPENSSL_DIR})
  if(DEFINED ENV{OPENSSL_LIB})
    set(OPENSSL_LIB $ENV{OPENSSL_LIB})
  else()
    set(OPENSSL_LIB $ENV{OPENSSL_DIR})
  endif()
  set(OpenSSL_INCLUDE_DIRS $ENV{OPENSSL_DIR}/include)
  set(OPENSSL_LIBRARIES ${OPENSSL_LIB}/libssl.so ${OPENSSL_LIB}/libcrypto.so)
  add_definitions(-isystem ${OpenSSL_INCLUDE_DIRS})
else()
  find_package(OpenSSL REQUIRED)
  message(STATUS "using OpenSSL version " ${OPENSSL_VERSION})
  add_definitions(-isystem /usr/include)
endif()

set(SRC_DIR ${PROJECT_SOURCE_DIR}/crypto)
set(CMPOSSL_SRC
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

set(INC_DIR ${PROJECT_SOURCE_DIR}/include/cmp)
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

add_library(${PROJECT_NAME} SHARED ${CMPOSSL_SRC})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE ${INC_DIR})
# does not work for OpenSSL 3.0+:
# add_definitions(-isystem ${INC_DIR})

if((CMAKE_BUILD_TYPE MATCHES Release) OR (DEFINED ENV{NDEBUG}))
  message(STATUS "build mode: Release")
  add_definitions(-DNDEBUG=1 -O2)
else()
  message(STATUS "build mode: Debug")
  add_definitions(-g -O0)
  set(DEBUG_FLAGS -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
  add_definitions(${DEBUG_FLAGS})
  link_libraries(${DEBUG_FLAGS})
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
  -include ${INC_DIR}/openssl/openssl_backport.h -DCMP_STANDALONE
  -DDEBUG_UNUSED -DPEDANTIC -pedantic -Wall -Wextra
  -Wswitch -Wsign-compare -Wmissing-prototypes -Wstrict-prototypes
  -Wshadow -Wformat -Wtype-limits -Wundef)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 90)

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${CMPOSSL_VERSION} SOVERSION ${CMPOSSL_VERSION_MAJOR}
  PUBLIC_HEADER "${INC_PUBLIC_HDRS}"
)

if(NOT TARGET build)
  add_custom_target(build COMMAND make all)
endif()

if(DEFINED ENV{ROOTFS})
  set(CMAKE_INSTALL_PREFIX $ENV{ROOTFS} CACHE PATH "comment" FORCE)
endif()

install(DIRECTORY doc/man3
  DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}
  )

install(TARGETS ${PROJECT_NAME}
  LIBRARY
    DESTINATION lib
    COMPONENT Library
  PUBLIC_HEADER
    DESTINATION include/cmp/openssl/
    COMPONENT Development
)

if(NOT TARGET uninstall)
  add_custom_target(uninstall COMMAND xargs -I{} rm -vf $ENV{DESTDIR}{} <install_manifest.txt)
endif()
