update_git_submodule(${THIRD_PARTY_DIR}/curl "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/curl CURL_VERSION)
get_submodule_remote_url(third-party/curl CURL_SOURCE_URL)

set(CURL_PROJECT_GENERIC_NAME curl)
set(CURL_PROJECT_GENERIC_NAMESPACE CURL)
set(CURL_ARTIFACT_NAME libcurl)

function(build_curl PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${CURL_PROJECT_GENERIC_NAME} ${CURL_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${CURL_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${CURL_ARTIFACT_NAME}.a)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -march=broadwell -g0 -Wno-deprecated-declarations ${extra_compile_flags}")

    # Suppress compiler-specific warnings caused by -O3
    if(COMPILER_CLANG)
        set(compile_flags "${compile_flags} -Wno-string-plus-int")
    else()
        set(compile_flags "${compile_flags} -Wno-stringop-truncation -Wno-maybe-uninitialized -Wno-stringop-overflow")
    endif()

    message(STATUS "CURL Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${CURL_VERSION}
        Source:         ${CURL_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    # The configuration has been based on:
    # https://github.com/VKCOM/curl/commit/00364cc6d672d9271032dbfbae3cfbc5e5f8542c
    # ./configure --prefix=/opt/curl7600 --without-librtmp --without-libssh2 --disable-ldap --disable-ldaps --disable-threaded-resolver --with-nghttp2 --enable-versioned-symbols
    set(cmake_args
            -DCMAKE_C_FLAGS=${compile_flags}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
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
            -DOPENSSL_ROOT_DIR=${OPENSSL_${pic_lib_specifier}_ROOT_DIR}
            -DOPENSSL_LIBRARIES=${OPENSSL_${pic_lib_specifier}_LIBRARIES}
            -DOPENSSL_INCLUDE_DIR=${OPENSSL_${pic_lib_specifier}_INCLUDE_DIR}
            -DCURL_ZLIB=ON
            -DZLIB_FOUND=ON
            -DCURL_SPECIAL_LIBZ=ON
            -DZLIB_ROOT=${ZLIB_${pic_lib_specifier}_ROOT}
            -DZLIB_LIBRARIES=${ZLIB_${pic_lib_specifier}_LIBRARIES}
            -DZLIB_INCLUDE_DIR=${ZLIB_${pic_lib_specifier}_INCLUDE_DIRS}/zlib
            -DNGHTTP2_FOUND=ON
            -DNGHTTP2_ROOT=${NGHTTP2_${pic_lib_specifier}_ROOT}
            -DNGHTTP2_LIBRARY=${NGHTTP2_${pic_lib_specifier}_LIBRARIES}
            -DNGHTTP2_INCLUDE_DIR=${NGHTTP2_${pic_lib_specifier}_INCLUDE_DIRS}
    )

    ExternalProject_Add(
            ${project_name}
            DEPENDS OpenSSL::${pic_namespace}::Crypto OpenSSL::${pic_namespace}::SSL ZLIB::${pic_namespace}::zlib NGHTTP2::${pic_namespace}::nghttp2
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BUILD_BYPRODUCTS ${libraries}
            CONFIGURE_COMMAND
                COMMAND ${CMAKE_COMMAND} ${cmake_args} -S ${source_dir} -B ${build_dir} -Wno-dev
            BUILD_COMMAND
                COMMAND ${CMAKE_COMMAND} --build ${build_dir} --config $<CONFIG> -j
            INSTALL_COMMAND
                COMMAND ${CMAKE_COMMAND} --install ${build_dir} --prefix ${install_dir} --config $<CONFIG>
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the curl is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that curl has been installed
    set(${CURL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${CURL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${CURL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_curl(OFF)
# PIC is ON
build_curl(ON)

set(CURL_FOUND ON)
