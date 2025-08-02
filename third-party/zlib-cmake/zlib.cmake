update_git_submodule(${THIRD_PARTY_DIR}/zlib "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/zlib ZLIB_VERSION)
get_submodule_remote_url(third-party/zlib ZLIB_SOURCE_URL)

set(ZLIB_PROJECT_GENERIC_NAME zlib)
set(ZLIB_PROJECT_GENERIC_NAMESPACE ZLIB)
set(ZLIB_ARTIFACT_NAME libz)

function(build_zlib PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${ZLIB_PROJECT_GENERIC_NAME} ${ZLIB_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${ZLIB_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${ZLIB_ARTIFACT_NAME}.a)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/zlib/1%3A1.3.dfsg%2Breally1.3.1-1/debian/rules/#L20
    set(compile_flags "$ENV{CFLAGS} -march=cascadelake -g0 -Wall -O3 -D_REENTRANT ${extra_compile_flags}")

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
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the zlib is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that zlib has been installed
    set(${ZLIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${ZLIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${ZLIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_zlib(OFF)
# PIC is ON
build_zlib(ON)

set(ZLIB_FOUND ON)
