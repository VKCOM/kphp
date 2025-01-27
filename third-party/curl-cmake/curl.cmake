update_git_submodule(${THIRD_PARTY_DIR}/curl)

set(CURL_INSTALL_DIR ${CMAKE_BINARY_DIR}/third-party/curl)
# Ensure the installation directory exists
file(MAKE_DIRECTORY ${CURL_INSTALL_DIR})

set(CURL_COMPILE_FLAGS "-O3 -Wno-deprecated-declarations")
# Suppress compiler-specific warnings caused by -O3
if(COMPILER_CLANG)
    set(CURL_COMPILE_FLAGS "${CURL_COMPILE_FLAGS} -Wno-string-plus-int")
else()
    set(CURL_COMPILE_FLAGS "${CURL_COMPILE_FLAGS} -Wno-stringop-truncation -Wno-maybe-uninitialized")
endif()

# The configuration has been based on:
# https://github.com/VKCOM/curl/commit/00364cc6d672d9271032dbfbae3cfbc5e5f8542c
# ./configure --prefix=/opt/curl7600 --without-librtmp --without-libssh2 --disable-ldap --disable-ldaps --disable-threaded-resolver --with-nghttp2 --enable-versioned-symbols
ExternalProject_Add(
        curl
        SOURCE_DIR ${THIRD_PARTY_DIR}/curl
        CMAKE_ARGS
        -DBUILD_TESTING=OFF
        -DCURL_WERROR=OFF                   # Recommend to enable when optimization level less than -O3
        -DBUILD_CURL_EXE=OFF
        -DCURL_STATICLIB=ON
        -DUSE_LIBRTMP=OFF                   # Disable RTMP support.
        -DCMAKE_USE_LIBSSH2=OFF             # Disable libssh2 support.
        -DCURL_DISABLE_LDAP=ON              # Disable LDAP support.
        -DCURL_DISABLE_LDAPS=ON             # Disable LDAPS support.
        -DENABLE_THREADED_RESOLVER=OFF      # Disable threaded resolver.
        -DUSE_NGHTTP2=ON                    # Enable HTTP/2 support.
        -DCMAKE_USE_OPENSSL=ON
        -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
        -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}
        -DCMAKE_INSTALL_PREFIX=${CURL_INSTALL_DIR}
        -DCMAKE_C_FLAGS=${CURL_COMPILE_FLAGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        BUILD_BYPRODUCTS ${CURL_INSTALL_DIR}/lib/libcurl.a
        INSTALL_COMMAND
        ${CMAKE_COMMAND}  --install . --config $<CONFIG> &&
        ${CMAKE_COMMAND} -E copy_directory ${CURL_INSTALL_DIR}/include ${OBJS_DIR}/include &&
        ${CMAKE_COMMAND} -E copy ${CURL_INSTALL_DIR}/lib/libcurl.a ${OBJS_DIR}/lib/
        BUILD_IN_SOURCE 1
)

set(CURL_INCLUDE_DIR ${CURL_INSTALL_DIR}/include)
# Ensure the include directory exists
file(MAKE_DIRECTORY ${CURL_INCLUDE_DIR})

add_library(CURL::curl STATIC IMPORTED)
set_target_properties(CURL::curl PROPERTIES
        IMPORTED_LOCATION ${CURL_INSTALL_DIR}/lib/libcurl.a
        INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIR}
)

# Set variables indicating that curl has been installed
set(CURL_FOUND ON)
set(CURL_LIBRARIES ${CURL_INSTALL_DIR}/lib/libcurl.a)
