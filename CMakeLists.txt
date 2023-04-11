cmake_minimum_required(VERSION 3.7)

project(cmp)

if(NOT DEFINED CMPOSSL_VERSION)
  set(CMPOSSL_VERSION_MAJOR 2)
  set(CMPOSSL_VERSION ${CMPOSSL_VERSION_MAJOR}.0)
endif()

message(STATUS "CMPforOpenSSL version " ${CMPOSSL_VERSION})

# set(CMAKE_VERBOSE_MAKEFILE ON)

if(DEFINED ENV{OPENSSL_DIR})
  set(OpenSSL_INCLUDE_DIRS $ENV{OPENSSL_DIR}/include)
  set(OPENSSL_LIBRARIES $ENV{OPENSSL_DIR}/libssl.so $ENV{OPENSSL_DIR}/libcrypto.so)
  add_definitions(-isystem ${OpenSSL_INCLUDE_DIRS})
else()
  find_package(OpenSSL REQUIRED)
endif()

set(INC_OPENSSL 	${PROJECT_SOURCE_DIR}/include/openssl)
set(INC_INTERNAL	${PROJECT_SOURCE_DIR}/include/internal)
set(LIBCMP_INC 		${PROJECT_SOURCE_DIR}/include/cmp)
set(LIBCMP_INC_OPENSSL  ${LIBCMP_INC}/openssl)
set(LIBCMP_INC_INTERNAL ${LIBCMP_INC}/internal)

# TODO remove 
file(GLOB INC_OPENSSL_HDRS  "${INC_OPENSSL}/*.h") 
file(MAKE_DIRECTORY ${LIBCMP_INC_OPENSSL}) 
file(COPY ${INC_OPENSSL_HDRS} DESTINATION ${LIBCMP_INC_OPENSSL}) 

file(GLOB INC_INTERNAL_HDRS "${INC_INTERNAL}/*.h")
file(MAKE_DIRECTORY ${LIBCMP_INC_INTERNAL})
file(COPY ${INC_INTERNAL_HDRS} DESTINATION ${LIBCMP_INC_INTERNAL})

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
)

add_library(${PROJECT_NAME} SHARED ${CMPOSSL_SRC})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_DIR})

if(DEFINED ENV{NDEBUG})
  add_definitions(-DNDEBUG=1 -O2)
else()
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

install(TARGETS ${PROJECT_NAME}
  LIBRARY
    DESTINATION lib
    COMPONENT Library
  PUBLIC_HEADER
    DESTINATION include/cmp/openssl/
    COMPONENT Development
)
