include_guard(GLOBAL)

function(string_to_list STRING_NAME LIST_NAME)
    string(REPLACE " " ";" "${LIST_NAME}" ${STRING_NAME})
endfunction()

function(vk_add_library)
    add_library(${ARGV})
    add_library(vk::${ARGV0} ALIAS ${ARGV0})
endfunction()

function(vk_add_library_pic)
    add_library(${ARGV})
    set_target_properties(${ARGV0} PROPERTIES
            POSITION_INDEPENDENT_CODE 1
            COMPILE_FLAGS "-fPIC"
    )
endfunction()

function(vk_add_library_no_pic)
    add_library(${ARGV})
    set_target_properties(${ARGV0} PROPERTIES
            POSITION_INDEPENDENT_CODE 0
            COMPILE_FLAGS "-fno-pic -static"
    )
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
            file(MAKE_DIRECTORY \$ENV{DESTDIR}${LINK_DIR})
            execute_process(
                COMMAND \${CMAKE_COMMAND} -E create_symlink ${TARGET_PATH} ${LINK_FILE_NAME}
                WORKING_DIRECTORY \$ENV{DESTDIR}${LINK_DIR}
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

# Function to initialize and update specific Git submodule
function(update_git_submodule SUBMODULE_PATH)
    message(STATUS "Updating Git submodule ${SUBMODULE_PATH} ...")

    # Update submodules
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init ${ARGN} ${SUBMODULE_PATH}
            WORKING_DIRECTORY ${BASE_DIR}
            RESULT_VARIABLE return_code
            OUTPUT_VARIABLE stdout
            ERROR_VARIABLE stderr
    )

    if(NOT return_code EQUAL 0)
        message(FATAL_ERROR "Failed to update Git submodule ${SUBMODULE_PATH}: ${stdout} ${stderr}")
    endif()
endfunction()

function(detect_xcode_sdk_path OUTPUT_VARIABLE)
    # Use execute_process to run the xcrun command and capture the output
    execute_process(
            COMMAND xcrun --sdk macosx --show-sdk-path
            OUTPUT_VARIABLE sdk_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_VARIABLE stderr
            RESULT_VARIABLE return_code
    )

    # Check if the command was successful
    if(return_code EQUAL 0)
        # Check if the SDK_PATH is not empty
        if(sdk_path)
            set(${OUTPUT_VARIABLE} "${sdk_path}" PARENT_SCOPE)
            message(STATUS "Detected Xcode SDK path: ${sdk_path}")
        else()
            message(FATAL_ERROR "Failed to detect Xcode SDK path: Output is empty.")
        endif()
    else()
        message(FATAL_ERROR "Failed to detect Xcode SDK path: ${stderr}")
    endif()
endfunction()

# Parameters:
#   TARGET - The initial target of runtime library whose dependencies will be combined.
#   COMBINED_TARGET - The name of the final static runtime library to be created.
function(combine_static_runtime_library TARGET COMBINED_TARGET)
    list(APPEND dependencies_list ${TARGET})

    # DFS for dependencies
    function(collect_dependencies ROOT_TARGET)
        set(target_linked_libraries_kind LINK_LIBRARIES)
        get_target_property(target_kind ${ROOT_TARGET} TYPE)
        if (${target_kind} STREQUAL "INTERFACE_LIBRARY")
            set(target_linked_libraries_kind INTERFACE_LINK_LIBRARIES)
        endif()

        # Get the list of linked libraries for the target.
        get_target_property(public_deps ${ROOT_TARGET} ${target_linked_libraries_kind})

        foreach(dep IN LISTS public_deps)
            if(TARGET ${dep})
                get_target_property(alias ${dep} ALIASED_TARGET)
                if(TARGET ${alias})
                    set(dep ${alias})
                endif()

                get_target_property(is_downloaded_lib ${dep} DOWNLOADED_LIBRARY)
                if(is_downloaded_lib EQUAL 1)
                    continue()
                endif()

                get_target_property(dep_kind ${dep} TYPE)
                if(${dep_kind} STREQUAL "STATIC_LIBRARY")
                    list(APPEND dependencies_list ${dep})
                endif()

                get_property(visited GLOBAL PROPERTY _${TARGET}_depends_on_${dep})
                if(NOT visited)
                    set_property(GLOBAL PROPERTY _${TARGET}_depends_on_${dep} ON)
                    collect_dependencies(${dep})
                endif()
            endif()
        endforeach()
        set(dependencies_list ${dependencies_list} PARENT_SCOPE)
    endfunction()

    # Start collecting dependencies from the initial target.
    collect_dependencies(${TARGET})
    list(REMOVE_DUPLICATES dependencies_list)

    set(combined_target_path ${OBJS_DIR}/lib${COMBINED_TARGET}.a)

    if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        file(WRITE ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar.in "CREATE ${combined_target_path}\n" )

        foreach(dep IN LISTS dependencies_list)
            file(APPEND ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar.in "ADDLIB $<TARGET_FILE:${dep}>\n")
        endforeach()
        file(APPEND ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar.in "SAVE\n")
        file(APPEND ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar.in "END\n")

        file(GENERATE
                OUTPUT ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar
                INPUT ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar.in
        )

        set(ar_tool ${CMAKE_AR})
        # If LTO is enabled
        if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        add_custom_command(
                COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar
                OUTPUT ${combined_target_path}
                COMMENT "Bundling ${COMBINED_TARGET}"
                VERBATIM
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^AppleClang$")
        set(ar_tool libtool)
        # If LTO is enabled
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        list(TRANSFORM dependencies_list PREPEND "$<TARGET_FILE:")
        list(TRANSFORM dependencies_list APPEND ">")

        add_custom_command(
                COMMAND cmake -E echo "$<GENEX_EVAL:$<JOIN:${dependencies_list},$<SEMICOLON>>>"
                COMMAND ${ar_tool} -static -o ${combined_target_path} "$<GENEX_EVAL:$<JOIN:${dependencies_list},$<SEMICOLON>>>"
                COMMAND_EXPAND_LISTS
                OUTPUT ${combined_target_path}
                COMMENT "Bundling ${COMBINED_TARGET}"
                VERBATIM
        )
    else()
        message(FATAL_ERROR "Unknown toolchain for runtime library combining")
    endif()

    add_custom_target(_combined_${TARGET} ALL DEPENDS ${combined_target_path})
    add_dependencies(_combined_${TARGET} ${TARGET})

    add_library(${COMBINED_TARGET} STATIC IMPORTED)
    set_target_properties(${COMBINED_TARGET}
            PROPERTIES
            IMPORTED_LOCATION ${combined_target_path}
            INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
    add_dependencies(${COMBINED_TARGET} _combined_${TARGET})
endfunction()
