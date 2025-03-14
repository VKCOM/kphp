update_git_submodule(${THIRD_PARTY_DIR}/numactl "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/numactl NUMACTL_VERSION)
get_submodule_remote_url(third-party/numactl NUMACTL_SOURCE_URL)

set(NUMACTL_PROJECT_GENERIC_NAME numactl)
set(NUMACTL_PROJECT_GENERIC_NAMESPACE NUMACTL)
set(NUMACTL_ARTIFACT_NAME libnuma)

function(build_numactl PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${NUMACTL_PROJECT_GENERIC_NAME} ${NUMACTL_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${NUMACTL_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${NUMACTL_ARTIFACT_NAME}.a)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/numactl/2.0.12-1/debian/rules/
    set(compile_flags "$ENV{CFLAGS} -Wno-unused-but-set-variable -g0 ${extra_compile_flags}")

    message(STATUS "NUMA Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${NUMACTL_VERSION}
        Source:         ${NUMACTL_SOURCE_URL}
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
                COMMAND ./autogen.sh
                COMMAND ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CFLAGS=${compile_flags} ./configure --prefix=${install_dir} --includedir=${install_dir}/include/numactl --disable-shared --enable-static
            BUILD_COMMAND
                COMMAND make libnuma.la -j
            INSTALL_COMMAND
                COMMAND make install-libLTLIBRARIES install-includeHEADERS
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${include_dirs} ${INCLUDE_DIR}
            BUILD_IN_SOURCE 0
    )

    add_library(${target_name} STATIC IMPORTED)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION ${libraries}
            INTERFACE_INCLUDE_DIRECTORIES ${include_dirs}
    )

    # Ensure that the NUMA is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that NUMA has been installed
    set(${NUMACTL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${NUMACTL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${NUMACTL_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_numactl(OFF)
# PIC is ON
build_numactl(ON)

set(NUMACTL_FOUND ON)
