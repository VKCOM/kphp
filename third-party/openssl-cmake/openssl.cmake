update_git_submodule(${THIRD_PARTY_DIR}/openssl "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/openssl OPENSSL_VERSION)
get_submodule_remote_url(third-party/openssl OPENSSL_SOURCE_URL)

set(OPENSSL_PROJECT_GENERIC_NAME openssl)
set(OPENSSL_PROJECT_GENERIC_NAMESPACE OpenSSL)
set(OPENSSL_FOUND_VARIABLE_PREFIX OPENSSL)
set(OPENSSL_CRYPTO_ARTIFACT_NAME libcrypto)
set(OPENSSL_SSL_ARTIFACT_NAME libssl)
set(OPENSSL_CRYPTO_TARGET Crypto)
set(OPENSSL_SSL_TARGET SSL)

function(build_openssl PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${OPENSSL_PROJECT_GENERIC_NAME} ${OPENSSL_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${OPENSSL_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(crypto_library  ${install_dir}/lib/${OPENSSL_CRYPTO_ARTIFACT_NAME}.a)
    set(ssl_library     ${install_dir}/lib/${OPENSSL_SSL_ARTIFACT_NAME}.a)
    set(libraries       ${crypto_library} ${ssl_library})
    set(patch_dir       ${build_dir}/debian/patches/)
    set(patch_series    ${build_dir}/debian/patches/series)
    set(crypto_target   ${OPENSSL_PROJECT_GENERIC_NAMESPACE}::${pic_namespace}::${OPENSSL_CRYPTO_TARGET})
    set(ssl_target      ${OPENSSL_PROJECT_GENERIC_NAMESPACE}::${pic_namespace}::${OPENSSL_SSL_TARGET})
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -march=broadwell -g0 ${extra_compile_flags}")

    # The configuration has been based on:
    # https://packages.debian.org/buster/libssl1.1
    #
    #   CONFARGS  = --prefix=/usr --openssldir=/usr/lib/ssl --libdir=lib/$(DEB_HOST_MULTIARCH) no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms
    #   ...
    #   ifeq ($(DEB_HOST_ARCH_CPU), amd64)
    #        CONFARGS += enable-ec_nistp_64_gcc_128
    #   endif
    #
    set(configure_flags --openssldir=/usr/lib/ssl no-shared no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms no-tests)
    if (NOT PIC_ENABLED)
        set(configure_flags ${configure_flags} no-pic -static)
    endif()
     if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(configure_flags ${configure_flags} enable-ec_nistp_64_gcc_128)
    endif()

    message(STATUS "OpenSSL Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${OPENSSL_VERSION}
        Source:         ${OPENSSL_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Crypto lib:     ${crypto_library}
        Crypto target:  ${crypto_target}
        SSL lib:        ${ssl_library}
        SSL target:     ${ssl_target}
        Configure:
          Flags:        ${configure_flags}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    ExternalProject_Add(
            ${project_name}
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${build_dir}
            BUILD_BYPRODUCTS ${libraries}
            PATCH_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${build_dir}
                COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build_dir} -DPATCH_SERIES=${patch_series} -DPATCH_DIR=${patch_dir} -P ../../cmake/apply_patches.cmake
            CONFIGURE_COMMAND
                COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CFLAGS=${compile_flags} ./config --prefix=${install_dir} ${configure_flags}
            BUILD_COMMAND
                COMMAND make build_libs -j
            INSTALL_COMMAND
                COMMAND make install_dev
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${crypto_target} STATIC IMPORTED)
    set_target_properties(${crypto_target} PROPERTIES
            IMPORTED_LOCATION ${crypto_library}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    add_library(${ssl_target} STATIC IMPORTED)
    set_target_properties(${ssl_target} PROPERTIES
            IMPORTED_LOCATION ${ssl_library}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the ssl and crypto are built before they are used
    add_dependencies(${crypto_target} ${project_name})
    add_dependencies(${ssl_target} ${project_name})

    # Set variables indicating that OpenSSL has been installed
    set(${OPENSSL_FOUND_VARIABLE_PREFIX}_${pic_lib_specifier}_ROOT_DIR ${install_dir} PARENT_SCOPE)
    set(${OPENSSL_FOUND_VARIABLE_PREFIX}_${pic_lib_specifier}_INCLUDE_DIR ${include_dirs} PARENT_SCOPE)
    set(${OPENSSL_FOUND_VARIABLE_PREFIX}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
    set(${OPENSSL_FOUND_VARIABLE_PREFIX}_${pic_lib_specifier}_CRYPTO_LIBRARY ${crypto_library} PARENT_SCOPE)
    set(${OPENSSL_FOUND_VARIABLE_PREFIX}_${pic_lib_specifier}_SSL_LIBRARY ${ssl_library} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_openssl(OFF)
# PIC is ON
build_openssl(ON)

set(OPENSSL_FOUND ON)
set(OPENSSL_USE_STATIC_LIBS ON)
