include_guard(GLOBAL)

function(string_to_list STRING_NAME LIST_NAME)
    string(REPLACE " " ";" "${LIST_NAME}" ${STRING_NAME})
endfunction()

function(vk_add_library)
    add_library(${ARGV})
    add_library(vk::${ARGV0} ALIAS ${ARGV0})
endfunction()

function(prepend VAR_NAME PREFIX)
    list(TRANSFORM ARGN PREPEND ${PREFIX} OUTPUT_VARIABLE ${VAR_NAME})
    set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)
endfunction(prepend)

function(prepare_cross_platform_libs VAR_NAME)
    set(${VAR_NAME} ${ARGN})
    if(NOT APPLE)
        list(TRANSFORM ${VAR_NAME} PREPEND -l:lib OUTPUT_VARIABLE ${VAR_NAME})
        list(TRANSFORM ${VAR_NAME} APPEND .a OUTPUT_VARIABLE ${VAR_NAME})
    endif()
    set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)
endfunction(prepare_cross_platform_libs)

function(install_symlink TARGET_PATH LINK COMPONENT_NAME)
    get_filename_component(LINK_DIR ${LINK} DIRECTORY)
    get_filename_component(LINK_FILE_NAME ${LINK} NAME)

    install(CODE "
            file(MAKE_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${LINK_DIR})
            execute_process(
                COMMAND \${CMAKE_COMMAND} -E create_symlink \${CMAKE_INSTALL_PREFIX}/${TARGET_PATH} ${LINK_FILE_NAME}
                WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${LINK_DIR}
            )
        "
        COMPONENT ${COMPONENT_NAME})
endfunction()

function(allow_deprecated_declarations)
    foreach(src_file ${ARGN})
        set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
    endforeach()
endfunction()

function(allow_deprecated_declarations_for_apple)
    if(APPLE)
        allow_deprecated_declarations(${ARGN})
    endif()
endfunction()

function(check_compiler_version compiler_name compiler_version)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${compiler_version})
        message(FATAL_ERROR "${compiler_name} version ${compiler_version} required at least, "
                "you run at ${CMAKE_CXX_COMPILER_VERSION}!")
    endif()
endfunction(check_compiler_version)

# Function to initialize and update Git submodules
function(update_git_submodules)
        message(STATUS "Updating Git submodules...")

        # Update submodules
        execute_process(
            COMMAND git submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE update_result
            ERROR_QUIET
        )

        if(NOT update_result EQUAL 0)
            message(FATAL_ERROR "Failed to update Git submodules.")
        endif()
endfunction()
