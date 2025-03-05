update_git_submodule(${THIRD_PARTY_DIR}/pcre "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/pcre PCRE_VERSION)
get_submodule_remote_url(third-party/pcre PCRE_SOURCE_URL)

set(PCRE_PROJECT_GENERIC_NAME pcre)
set(PCRE_PROJECT_GENERIC_NAMESPACE PCRE)
set(PCRE_ARTIFACT_NAME libpcre)

function(build_pcre PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${PCRE_PROJECT_GENERIC_NAME} ${PCRE_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${PCRE_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${PCRE_ARTIFACT_NAME}.a)
    set(patch_dir       ${build_dir}/debian/patches/)
    set(patch_series    ${build_dir}/debian/patches/series)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -g0 -Wall ${extra_compile_flags}")

    message(STATUS "PCRE Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${PCRE_VERSION}
        Source:         ${PCRE_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    # The configuration has been based on:
    # https://sources.debian.org/src/pcre3/2%3A8.39-12/debian/rules/#L30
    set(cmake_args
            -DCMAKE_C_FLAGS=${compile_flags}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_FLAGS=${compile_flags}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DBUILD_STATIC_LIBS=ON
            -DBUILD_SHARED_LIBS=OFF
            -DPCRE_BUILD_PCRE8=ON
            -DPCRE_BUILD_PCRECPP=OFF
            -DPCRE_BUILD_PCREGREP=OFF
            -DPCRE_BUILD_TESTS=OFF
            -DPCRE_REBUILD_CHARTABLES=OFF
            -DPCRE_SUPPORT_JIT=ON
            -DPCRE_SUPPORT_UTF=ON
            -DPCRE_SUPPORT_UNICODE_PROPERTIES=ON
            -DPCRE_SUPPORT_LIBZ=ON
            -DZLIB_FOUND=ON
            -DZLIB_ROOT=${ZLIB_${pic_lib_specifier}_ROOT}
            -DZLIB_LIBRARIES=${ZLIB_${pic_lib_specifier}_LIBRARIES}
            -DZLIB_INCLUDE_DIR=${ZLIB_${pic_lib_specifier}_INCLUDE_DIRS}/zlib
    )

    ExternalProject_Add(
            ${project_name}
            DEPENDS ZLIB::${pic_namespace}::zlib
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
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${include_dirs}/${PCRE_PROJECT_GENERIC_NAME}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs}/${PCRE_PROJECT_GENERIC_NAME} ${INCLUDE_DIR}/${PCRE_PROJECT_GENERIC_NAME}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the PCRE is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that PCRE has been installed
    set(${PCRE_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${PCRE_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${PCRE_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_pcre(OFF)
# PIC is ON
build_pcre(ON)

set(PCRE_FOUND ON)
