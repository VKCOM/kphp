update_git_submodule(${THIRD_PARTY_DIR}/yaml-cpp "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/yaml-cpp YAML_CPP_VERSION)
get_submodule_remote_url(third-party/yaml-cpp YAML_CPP_SOURCE_URL)

set(YAML_CPP_PROJECT_GENERIC_NAME yaml-cpp)
set(YAML_CPP_PROJECT_GENERIC_NAMESPACE YAML_CPP)
set(YAML_CPP_ARTIFACT_NAME libyaml-cpp)

function(build_yaml_cpp PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${YAML_CPP_PROJECT_GENERIC_NAME} ${YAML_CPP_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${YAML_CPP_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${YAML_CPP_ARTIFACT_NAME}.a)
    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -g0 ${extra_compile_flags}")

    # Suppress compiler-specific warnings
    if(COMPILER_CLANG)
        set(compile_flags "${compile_flags}")
    else()
        set(compile_flags "${compile_flags} -Wno-maybe-uninitialized")
    endif()

    message(STATUS "YAML-cpp Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${YAML_CPP_VERSION}
        Source:         ${YAML_CPP_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          CXX compiler: ${CMAKE_CXX_COMPILER}
          CXX flags:    ${compile_flags}
    ")

    # The configuration has been based on:
    # https://sources.debian.org/src/yaml-cpp/0.6.2-4/debian/rules/
    set(cmake_args
            -DCMAKE_C_FLAGS=${compile_flags}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_FLAGS=${compile_flags}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DYAML_CPP_BUILD_TESTS=OFF
            -DYAML_CPP_BUILD_TOOLS=OFF
            -DYAML_CPP_BUILD_CONTRIB=OFF
            -DBUILD_SHARED_LIBS=OFF
    )

    ExternalProject_Add(
            ${project_name}
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${build_dir}
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

    # Ensure that the YAML-cpp is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that YAML-cpp has been installed
    set(${YAML_CPP_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${YAML_CPP_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${YAML_CPP_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_yaml_cpp(OFF)
# PIC is ON
build_yaml_cpp(ON)

set(YAML_CPP_FOUND ON)
