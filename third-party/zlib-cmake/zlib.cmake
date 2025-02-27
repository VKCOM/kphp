update_git_submodule(${THIRD_PARTY_DIR}/zlib "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/zlib ZLIB_VERSION)
get_submodule_remote_url(third-party/zlib ZLIB_SOURCE_URL)

function(build_zlib PIC_ENABLED)
    set(project_generic_name zlib)
    set(target_namespace ZLIB)
    set(artifact_name libz)

    make_third_party_configuration(${PIC_ENABLED} ${project_generic_name} ${target_namespace} ${artifact_name}
            project_name
            extra_compile_flags
            target_name
            lib_prefix
            archive_name
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${project_generic_name})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${archive_name})
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/zlib/1%3A1.3.dfsg%2Breally1.3.1-1/debian/rules/#L20
    set(compile_flags "$ENV{CFLAGS} -g0 -Wall -O3 -D_REENTRANT ${extra_compile_flags}")

    message(STATUS "Zlib Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${ZLIB_VERSION}
        Source:         ${ZLIB_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
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
            CONFIGURE_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${build_dir}
                COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CFLAGS=${compile_flags} ./configure --prefix=${install_dir} --includedir=${include_dirs}/zlib --static
            BUILD_COMMAND
                COMMAND make libz.a -j
            INSTALL_COMMAND
                COMMAND make install
                COMMAND ${CMAKE_COMMAND} -E copy ${install_dir}/lib/libz.a ${libraries}
                COMMAND ${CMAKE_COMMAND} -E copy ${libraries} ${LIB_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the zlib are built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that zlib has been installed
    set(${lib_prefix}ROOT ${install_dir} PARENT_SCOPE)
    set(${lib_prefix}INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${lib_prefix}LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_zlib(OFF)
# PIC is ON
build_zlib(ON)

set(ZLIB_FOUND ON)
