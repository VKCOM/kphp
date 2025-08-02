update_git_submodule(${THIRD_PARTY_DIR}/uber-h3 "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/uber-h3 UBER_H3_VERSION)
get_submodule_remote_url(third-party/uber-h3 UBER_H3_SOURCE_URL)

set(UBER_H3_PROJECT_GENERIC_NAME uber-h3)
set(UBER_H3_PROJECT_GENERIC_NAMESPACE UBER_H3)
set(UBER_H3_ARTIFACT_NAME libh3)

function(build_uber_h3 PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${UBER_H3_PROJECT_GENERIC_NAME} ${UBER_H3_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir      ${THIRD_PARTY_DIR}/${UBER_H3_PROJECT_GENERIC_NAME})
    set(build_dir       ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs    ${install_dir}/include)
    set(libraries       ${install_dir}/lib/${UBER_H3_ARTIFACT_NAME}.a)

    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -march=icelake-server -g0 ${extra_compile_flags}")

    message(STATUS "Uber-h3 Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${UBER_H3_VERSION}
        Source:         ${UBER_H3_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          C compiler:   ${CMAKE_C_COMPILER}
          CFLAGS:       ${compile_flags}
    ")

    set(cmake_args
            -DCMAKE_C_FLAGS=${compile_flags}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DENABLE_COVERAGE=OFF
            -DBUILD_BENCHMARKS=OFF
            -DBUILD_FILTERS=OFF
            -DBUILD_GENERATORS=OFF
            -DENABLE_LINTING=OFF
            -DENABLE_DOCS=OFF
            -DBUILD_TESTING=OFF
            -DENABLE_FORMAT=OFF
            -DCLANG_FORMAT_PATH=1   # for useless warning prevention
            -DCLANG_TIDY_PATH=1     # for useless warning prevention
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

    # Ensure that the Uber-h3 is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that Uber-h3 has been installed
    set(${UBER_H3_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${UBER_H3_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${UBER_H3_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_uber_h3(OFF)
# PIC is ON
build_uber_h3(ON)

set(UBER_H3_FOUND ON)
