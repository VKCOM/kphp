update_git_submodule(${THIRD_PARTY_DIR}/curl "--remote")

set(CURL_SOURCE_DIR     ${THIRD_PARTY_DIR}/curl)
set(CURL_BUILD_DIR      ${CMAKE_BINARY_DIR}/third-party/curl/build)
set(CURL_INSTALL_DIR    ${CMAKE_BINARY_DIR}/third-party/curl/install)
set(CURL_LIBRARIES      ${CURL_INSTALL_DIR}/lib/libcurl.a)
set(CURL_INCLUDE_DIRS   ${CURL_INSTALL_DIR}/include)
# Ensure the build, installation and "include" directories exists
file(MAKE_DIRECTORY ${CURL_BUILD_DIR})
file(MAKE_DIRECTORY ${CURL_INSTALL_DIR})
file(MAKE_DIRECTORY ${CURL_INCLUDE_DIRS})

set(CURL_COMPILE_FLAGS "$ENV{CFLAGS} -g0 -fno-pic -Wno-deprecated-declarations")
if(NOT APPLE)
    set(CURL_COMPILE_FLAGS "${CURL_COMPILE_FLAGS} -static")
endif()

# Suppress compiler-specific warnings caused by -O3
if(COMPILER_CLANG)
    set(CURL_COMPILE_FLAGS "${CURL_COMPILE_FLAGS} -Wno-string-plus-int")
else()
    set(CURL_COMPILE_FLAGS "${CURL_COMPILE_FLAGS} -Wno-stringop-truncation -Wno-maybe-uninitialized -Wno-stringop-overflow")
endif()

# The configuration has been based on:
# https://github.com/VKCOM/curl/commit/00364cc6d672d9271032dbfbae3cfbc5e5f8542c
# ./configure --prefix=/opt/curl7600 --without-librtmp --without-libssh2 --disable-ldap --disable-ldaps --disable-threaded-resolver --with-nghttp2 --enable-versioned-symbols
set(CURL_CMAKE_ARGS
        -DCMAKE_C_FLAGS=${CURL_COMPILE_FLAGS}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_FLAGS=${CURL_COMPILE_FLAGS}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE=OFF
        -DBUILD_TESTING=OFF
        -DCURL_WERROR=ON                    # Recommend to enable when optimization level less than -O3
        -DBUILD_CURL_EXE=OFF
        -DCURL_STATICLIB=ON
        -DUSE_LIBRTMP=OFF                   # Disable RTMP support.
        -DCMAKE_USE_LIBSSH2=OFF             # Disable libssh2 support.
        -DCURL_DISABLE_LDAP=ON              # Disable LDAP support.
        -DCURL_DISABLE_LDAPS=ON             # Disable LDAPS support.
        -DENABLE_THREADED_RESOLVER=OFF      # Disable threaded resolver.
        -DUSE_NGHTTP2=ON                    # Enable HTTP/2 support.
        -DCMAKE_USE_OPENSSL=ON
        -DOPENSSL_FOUND=ON
        -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
        -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}
        -DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}
        -DCURL_ZLIB=ON
        -DZLIB_FOUND=ON
        -DCURL_SPECIAL_LIBZ=ON
        -DZLIB_ROOT=${ZLIB_NO_PIC_ROOT}
        -DZLIB_LIBRARIES=${ZLIB_NO_PIC_LIBRARIES}
        -DZLIB_INCLUDE_DIR=${ZLIB_NO_PIC_INCLUDE_DIRS}/zlib
        -DNGHTTP2_FOUND=ON
        -DNGHTTP2_ROOT=${NGHTTP2_ROOT}
        -DNGHTTP2_LIBRARY=${NGHTTP2_LIBRARIES}
        -DNGHTTP2_INCLUDE_DIR=${NGHTTP2_INCLUDE_DIRS}
)

ExternalProject_Add(
        curl
        DEPENDS openssl zlib-no-pic nghttp2
        PREFIX ${CURL_BUILD_DIR}
        SOURCE_DIR ${CURL_SOURCE_DIR}
        INSTALL_DIR ${CURL_INSTALL_DIR}
        BUILD_BYPRODUCTS ${CURL_INSTALL_DIR}/lib/libcurl.a
        CONFIGURE_COMMAND
            COMMAND ${CMAKE_COMMAND} ${CURL_CMAKE_ARGS} -S ${CURL_SOURCE_DIR} -B ${CURL_BUILD_DIR} -Wno-dev
        BUILD_COMMAND
            COMMAND ${CMAKE_COMMAND} --build ${CURL_BUILD_DIR} --config $<CONFIG> -j
        INSTALL_COMMAND
            COMMAND ${CMAKE_COMMAND} --install ${CURL_BUILD_DIR} --prefix ${CURL_INSTALL_DIR} --config $<CONFIG>
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CURL_INSTALL_DIR}/include ${INCLUDE_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${CURL_INSTALL_DIR}/lib/libcurl.a ${LIB_DIR}
        BUILD_IN_SOURCE 0
)

add_library(CURL::curl STATIC IMPORTED)
set_target_properties(CURL::curl PROPERTIES
        IMPORTED_LOCATION ${CURL_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIRS}
)

# Set variables indicating that curl has been installed
set(CURL_FOUND ON)

cmake_print_variables(CURL_LIBRARIES)
