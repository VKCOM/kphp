update_git_submodule(${THIRD_PARTY_DIR}/openssl "--remote")

set(OPENSSL_SOURCE_DIR  ${THIRD_PARTY_DIR}/openssl)
set(OPENSSL_BUILD_DIR   ${CMAKE_BINARY_DIR}/third-party/openssl/build)
set(OPENSSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/third-party/openssl/install)
# Ensure the build and installation directories exists
file(MAKE_DIRECTORY ${OPENSSL_BUILD_DIR})
file(MAKE_DIRECTORY ${OPENSSL_INSTALL_DIR})

set(OPENSSL_COMPILE_FLAGS "$ENV{CFLAGS} -g0")

# The configuration has been based on:
# https://packages.debian.org/buster/libssl1.1
#
#   CONFARGS  = --prefix=/usr --openssldir=/usr/lib/ssl --libdir=lib/$(DEB_HOST_MULTIARCH) no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms
#   ...
#   ifeq ($(DEB_HOST_ARCH_CPU), amd64)
#        CONFARGS += enable-ec_nistp_64_gcc_128
#   endif
#
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(OPENSSL_CONFIGURE_EXTRA_OPTION enable-ec_nistp_64_gcc_128)
endif()

ExternalProject_Add(
        openssl
        PREFIX ${OPENSSL_BUILD_DIR}
        SOURCE_DIR ${OPENSSL_SOURCE_DIR}
        INSTALL_DIR ${OPENSSL_INSTALL_DIR}
        BINARY_DIR ${OPENSSL_BUILD_DIR}
        BUILD_BYPRODUCTS ${OPENSSL_INSTALL_DIR}/lib/libssl.a ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a
        CONFIGURE_COMMAND
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${OPENSSL_SOURCE_DIR} ${OPENSSL_BUILD_DIR}
            COMMAND ./config --prefix=${OPENSSL_INSTALL_DIR} --openssldir=/usr/lib/ssl no-shared no-pic no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms no-tests ${OPENSSL_CONFIGURE_EXTRA_OPTION}
        BUILD_COMMAND
            COMMAND ${CMAKE_COMMAND} -E env CFLAGS=${OPENSSL_COMPILE_FLAGS} make
        INSTALL_COMMAND
            COMMAND make install_sw
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${OPENSSL_INSTALL_DIR}/include ${INCLUDE_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_INSTALL_DIR}/lib/libssl.a ${LIB_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a ${LIB_DIR}
        BUILD_IN_SOURCE 0
)

set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include)
# Ensure the include directory exists
file(MAKE_DIRECTORY ${OPENSSL_INCLUDE_DIR})

add_library(OpenSSL::SSL STATIC IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.a
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR}
)

add_library(OpenSSL::Crypto STATIC IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR}
)

# Ensure that the OpenSSL libraries are built before they are used
add_dependencies(OpenSSL::SSL openssl)
add_dependencies(OpenSSL::Crypto openssl)

# Set variables indicating that OpenSSL has been found and specify its library locations
set(OPENSSL_FOUND ON)
set(OPENSSL_ROOT_DIR ${OPENSSL_INSTALL_DIR})
set(OPENSSL_LIBRARIES ${OPENSSL_INSTALL_DIR}/lib/libssl.a ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a)
set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a)
set(OPENSSL_SSL_LIBRARY ${OPENSSL_INSTALL_DIR}/lib/libssl.a)
set(OPENSSL_USE_STATIC_LIBS ON)

cmake_print_variables(OPENSSL_LIBRARIES)
