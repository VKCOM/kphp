update_git_submodule(${THIRD_PARTY_DIR}/zstd "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/zstd ZSTD_VERSION)
get_submodule_remote_url(third-party/zstd ZSTD_SOURCE_URL)

set(PROJECT_GENERIC_NAME zstd)
set(PROJECT_GENERIC_NAMESPACE ZSTD)
set(ARTIFACT_NAME libzstd)

function(build_zstd PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${PROJECT_GENERIC_NAME} ${PROJECT_GENERIC_NAMESPACE} ${ARTIFACT_NAME}
            project_name
            target_name
            extra_compile_flags
            archive_name
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(binary_dir      ${build_dir}/lib)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${archive_name})
    set(patch_dir       ${build_dir}/debian/patches/)
    set(patch_series    ${build_dir}/debian/patches/series)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/libzstd/1.4.8%2Bdfsg-2.1/debian/rules/
    set(compile_flags "$ENV{CFLAGS} -g0 -Wno-unused-but-set-variable ${extra_compile_flags}")

    message(STATUS "ZSTD Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${ZSTD_VERSION}
        Source:         ${ZSTD_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    set(make_args
            CC=${CMAKE_C_COMPILER}
            CFLAGS=${compile_flags}
    )

    set(make_install_args
            PREFIX=${install_dir}
            INCLUDEDIR=${install_dir}/include/zstd
    )

    ExternalProject_Add(
            ${project_name}
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${binary_dir}
            BUILD_BYPRODUCTS ${libraries}
            PATCH_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${build_dir}
                COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build_dir} -DPATCH_SERIES=${patch_series} -DPATCH_DIR=${patch_dir} -P ../../cmake/apply_patches.cmake
            CONFIGURE_COMMAND
                COMMAND # Nothing to configure
            BUILD_COMMAND
                COMMAND ${CMAKE_COMMAND} -E env ${make_args} make libzstd.a -j
            INSTALL_COMMAND
                COMMAND ${CMAKE_COMMAND} -E env ${make_install_args} make install-static install-includes
                COMMAND ${CMAKE_COMMAND} -E copy ${install_dir}/lib/${ARTIFACT_NAME}.a ${libraries}
                COMMAND ${CMAKE_COMMAND} -E copy ${libraries} ${LIB_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the ZSTD are built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that ZSTD has been installed
    set(${PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_zstd(OFF)
# PIC is ON
build_zstd(ON)

set(ZSTD_FOUND ON)
