update_git_submodule(${THIRD_PARTY_DIR}/nghttp2 "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/nghttp2 NGHTTP2_VERSION)
get_submodule_remote_url(third-party/nghttp2 NGHTTP2_SOURCE_URL)

set(NGHTTP2_PROJECT_GENERIC_NAME nghttp2)
set(NGHTTP2_PROJECT_GENERIC_NAMESPACE NGHTTP2)
set(NGHTTP2_ARTIFACT_NAME libnghttp2)

function(build_nghttp2 PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${NGHTTP2_PROJECT_GENERIC_NAME} ${NGHTTP2_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${NGHTTP2_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${NGHTTP2_ARTIFACT_NAME}.a)
    set(patch_dir       ${build_dir}/debian/patches/)
    set(patch_series    ${build_dir}/debian/patches/series)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/libzstd/1.4.8%2Bdfsg-2.1/debian/rules/
    set(compile_flags "$ENV{CFLAGS} -g0 ${extra_compile_flags}")

    message(STATUS "NGHTTP2 Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${NGHTTP2_VERSION}
        Source:         ${NGHTTP2_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    # The configuration has been based on:
    # https://salsa.debian.org/debian/nghttp2/-/blob/buster/debian/rules#L8
    set(cmake_args
            -DCMAKE_C_FLAGS=${compile_flags}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_FLAGS=${compile_flags}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DENABLE_WERROR=ON
            -DENABLE_THREADS=OFF
            -DENABLE_APP=OFF
            -DENABLE_HPACK_TOOLS=OFF
            -DENABLE_ASIO_LIB=OFF
            -DENABLE_EXAMPLES=OFF
            -DENABLE_PYTHON_BINDINGS=OFF
            -DENABLE_FAILMALLOC=OFF
            -DENABLE_LIB_ONLY=ON
            -DENABLE_STATIC_LIB=ON
            -DENABLE_SHARED_LIB=OFF
            -DWITH_LIBXML2=OFF
            -DWITH_JEMALLOC=OFF
            -DWITH_SPDYLAY=OFF
            -DWITH_MRUBY=OFF
            -DWITH_NEVERBLEED=OFF
            -DOPENSSL_FOUND=ON
            -DOPENSSL_ROOT_DIR=${OPENSSL_${pic_lib_specifier}_ROOT_DIR}
            -DOPENSSL_LIBRARIES=${OPENSSL_${pic_lib_specifier}_LIBRARIES}
            -DOPENSSL_INCLUDE_DIR=${OPENSSL_${pic_lib_specifier}_INCLUDE_DIR}
            -DZLIB_FOUND=ON
            -DZLIB_ROOT=${ZLIB_${pic_lib_specifier}_ROOT}
            -DZLIB_LIBRARIES=${ZLIB_${pic_lib_specifier}_LIBRARIES}
            -DZLIB_INCLUDE_DIR=${ZLIB_${pic_lib_specifier}_INCLUDE_DIRS}/zlib
    )

    ExternalProject_Add(
            ${project_name}
            DEPENDS OpenSSL::${pic_namespace}::Crypto OpenSSL::${pic_namespace}::SSL ZLIB::${pic_namespace}::zlib
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${build_dir}
            BUILD_BYPRODUCTS ${libraries}
            PATCH_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${build_dir}
                COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build_dir} -DPATCH_SERIES=${patch_series} -DPATCH_DIR=${patch_dir} -P ../../cmake/apply_patches.cmake
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

    # Ensure that the Nghttp2 are built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that Nghttp2 has been installed
    set(${NGHTTP2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${NGHTTP2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${NGHTTP2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_nghttp2(OFF)
# PIC is ON
build_nghttp2(ON)

set(NGHTTP2_FOUND ON)
