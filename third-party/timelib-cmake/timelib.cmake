find_program(RE2C_EXECUTABLE re2c)

if(NOT RE2C_EXECUTABLE)
    message(FATAL_ERROR "re2c is required but was not found. Please install re2c and try again.")
else()
    message(STATUS "Found re2c: ${RE2C_EXECUTABLE}")
endif()

update_git_submodule(${THIRD_PARTY_DIR}/timelib "--remote")
get_submodule_version(${THIRD_PARTY_DIR}/timelib TIMELIB_VERSION)
get_submodule_remote_url(third-party/timelib TIMELIB_SOURCE_URL)

set(TIMELIB_PROJECT_GENERIC_NAME timelib)
set(TIMELIB_PROJECT_GENERIC_NAMESPACE KPHP_TIMELIB)
set(TIMELIB_ARTIFACT_NAME libkphp-timelib)

function(build_timelib PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${TIMELIB_PROJECT_GENERIC_NAME} ${TIMELIB_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir          ${THIRD_PARTY_DIR}/${TIMELIB_PROJECT_GENERIC_NAME})
    set(patched_source_dir  ${CMAKE_BINARY_DIR}/third-party/${project_name}/source)
    set(build_dir           ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir         ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs        ${install_dir}/include)
    set(libraries           ${install_dir}/lib/${TIMELIB_ARTIFACT_NAME}.a)

    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${patched_source_dir})
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    set(compile_flags "$ENV{CFLAGS} -march=cascadelake -g0 ${extra_compile_flags}")

    message(STATUS "Timelib Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${TIMELIB_VERSION}
        Source:         ${TIMELIB_SOURCE_URL}
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
            -DCMAKE_CXX_FLAGS=${compile_flags}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DCMAKE_INSTALL_PREFIX=${install_dir}
    )

    ExternalProject_Add(
            ${project_name}
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${patched_source_dir}
            BUILD_BYPRODUCTS ${libraries}
            CONFIGURE_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${patched_source_dir}
                COMMAND ${CMAKE_COMMAND} ${cmake_args} -S ${patched_source_dir} -B ${build_dir} -Wno-dev
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

    # Ensure that the timelib is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that timelib has been installed
    set(${TIMELIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${TIMELIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${TIMELIB_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_timelib(OFF)
# PIC is ON
build_timelib(ON)

set(TIMELIB_FOUND ON)
