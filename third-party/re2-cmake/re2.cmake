update_git_submodule(${THIRD_PARTY_DIR}/re2 "--recursive")
get_submodule_version(${THIRD_PARTY_DIR}/re2 RE2_VERSION)
get_submodule_remote_url(third-party/re2 RE2_SOURCE_URL)

set(RE2_PROJECT_GENERIC_NAME re2)
set(RE2_PROJECT_GENERIC_NAMESPACE RE2)
set(RE2_ARTIFACT_NAME libre2)

function(build_re2 PIC_ENABLED)
    make_third_party_configuration(${PIC_ENABLED} ${RE2_PROJECT_GENERIC_NAME} ${RE2_PROJECT_GENERIC_NAMESPACE}
            project_name
            target_name
            extra_compile_flags
            pic_namespace
            pic_lib_specifier
    )

    set(source_dir              ${THIRD_PARTY_DIR}/${RE2_PROJECT_GENERIC_NAME})
    set(build_dir               ${CMAKE_BINARY_DIR}/third-party/${project_name}/build)
    set(install_dir             ${CMAKE_BINARY_DIR}/third-party/${project_name}/install)
    set(include_dirs            ${install_dir}/include)
    set(libraries               ${install_dir}/lib/${RE2_ARTIFACT_NAME}.a)
    set(vendor_patch_dir        ${build_dir}/debian/patches/)
    set(vendor_patch_series     ${build_dir}/debian/patches/series)
    set(template_patch_dir      ${THIRD_PARTY_DIR}/${RE2_PROJECT_GENERIC_NAME}-cmake/patches)
    set(template_patch_series   ${THIRD_PARTY_DIR}/${RE2_PROJECT_GENERIC_NAME}-cmake/patches/series)
    set(generated_patch_dir     ${CMAKE_BINARY_DIR}/third-party/${project_name}/patches/)
    set(generated_patch_series  ${CMAKE_BINARY_DIR}/third-party/${project_name}/patches/series)

    # Ensure the build, installation and "include" directories exists
    file(MAKE_DIRECTORY ${build_dir})
    file(MAKE_DIRECTORY ${install_dir})
    file(MAKE_DIRECTORY ${include_dirs})

    # The configuration has been based on:
    # https://sources.debian.org/src/re2/20190101%2Bdfsg-2/debian/rules/#L5
    set(compile_flags "$ENV{CFLAGS} -g0 -Wall -std=c++11 -O3 ${extra_compile_flags}")

    message(STATUS "RE2 Summary:

        PIC enabled:    ${PIC_ENABLED}
        Version:        ${RE2_VERSION}
        Source:         ${RE2_SOURCE_URL}
        Include dirs:   ${include_dirs}
        Libraries:      ${libraries}
        Target name:    ${target_name}
        Compiler:
          CXX compiler:   ${CMAKE_CXX_COMPILER}
          CXX flags:      ${compile_flags}
    ")

    set(cmake_args
            -DCMAKE_CXX_FLAGS=${compile_flags}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_POSITION_INDEPENDENT_CODE=${PIC_ENABLED}
            -DBUILD_SHARED_LIBS=OFF
            -DUSEPCRE=OFF
            -DRE2_BUILD_TESTING=OFF
    )

    # Resolve templated patches
    file(READ "${template_patch_series}" series_content)
    string(REPLACE "\n" ";" series_list "${series_content}")
    foreach(patch IN LISTS series_list)
        if(NOT patch STREQUAL "")
            configure_file(${template_patch_dir}/${patch}.template ${generated_patch_dir}/${patch})
        endif()
    endforeach()

    ExternalProject_Add(
            ${project_name}
            DEPENDS unicode-data
            PREFIX ${build_dir}
            SOURCE_DIR ${source_dir}
            INSTALL_DIR ${install_dir}
            BINARY_DIR ${build_dir}
            BUILD_BYPRODUCTS ${libraries}
            PATCH_COMMAND
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir} ${build_dir}
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${template_patch_dir} ${generated_patch_dir}
                # Vendor patches
                COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build_dir} -DPATCH_SERIES=${vendor_patch_series} -DPATCH_DIR=${vendor_patch_dir} -P ../../cmake/apply_patches.cmake
                # KPHP patches
                COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build_dir} -DPATCH_SERIES=${generated_patch_series} -DPATCH_DIR=${generated_patch_dir} -P ../../cmake/apply_patches.cmake
            CONFIGURE_COMMAND
                COMMAND ${Python3_EXECUTABLE} re2/make_unicode_casefold.py > re2/unicode_casefold.cc
                COMMAND ${Python3_EXECUTABLE} re2/make_unicode_groups.py > re2/unicode_groups.cc
                COMMAND ${PERL_EXECUTABLE} re2/make_perl_groups.pl > re2/perl_groups.cc
                COMMAND ${CMAKE_COMMAND} ${cmake_args} -S ${source_dir} -B ${build_dir}
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

    # Ensure that the RE2 is built before they are used
    add_dependencies(${target_name} ${project_name})

    # Set variables indicating that RE2 has been installed
    set(${RE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_ROOT ${install_dir} PARENT_SCOPE)
    set(${RE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
    set(${RE2_PROJECT_GENERIC_NAMESPACE}_${pic_lib_specifier}_LIBRARIES ${libraries} PARENT_SCOPE)
endfunction()

# PIC is OFF
build_re2(OFF)
# PIC is ON
build_re2(ON)

set(RE2_FOUND ON)
