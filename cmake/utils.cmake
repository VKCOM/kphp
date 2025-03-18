include_guard(GLOBAL)

function(string_to_list STRING_NAME LIST_NAME)
    string(REPLACE " " ";" "${LIST_NAME}" ${STRING_NAME})
endfunction()

function(check_vk_library_suffix TARGET SUFFIX)
    string(LENGTH "${SUFFIX}" suffix_length)
    string(LENGTH "${TARGET}" target_length)

    math(EXPR suffix_begin "${target_length} - ${suffix_length}")
    string(SUBSTRING "${TARGET}" ${suffix_begin} -1 expected_suffix)

    if(NOT expected_suffix STREQUAL "${SUFFIX}")
        message(FATAL_ERROR "vk library name '${TARGET}' does not end with the required suffix '${SUFFIX}'.")
    endif()
endfunction()

function(add_vk_library_to_pic_namespace TARGET)
    check_vk_library_suffix(${TARGET} ${PIC_LIBRARY_SUFFIX})

    string(REGEX REPLACE "${PIC_LIBRARY_SUFFIX}$" "" new_target "${TARGET}")
    add_library(vk::${PIC_NAMESPACE}::${new_target} ALIAS ${TARGET})
endfunction()

function(add_vk_library_to_no_pic_namespace TARGET)
    check_vk_library_suffix(${TARGET} ${NO_PIC_LIBRARY_SUFFIX})

    string(REGEX REPLACE "${NO_PIC_LIBRARY_SUFFIX}$" "" new_target "${TARGET}")
    add_library(vk::${NO_PIC_NAMESPACE}::${new_target} ALIAS ${TARGET})
endfunction()

function(vk_add_library_pic)
    add_library(${ARGV})
    set_target_properties(${ARGV0} PROPERTIES
            POSITION_INDEPENDENT_CODE 1
    )
    target_compile_options(${ARGV0} PRIVATE -fPIC)
    add_vk_library_to_pic_namespace(${ARGV0})
endfunction()

function(vk_add_library_no_pic)
    add_library(${ARGV})
    set_target_properties(${ARGV0} PROPERTIES
            POSITION_INDEPENDENT_CODE 0
    )
    target_compile_options(${ARGV0} PRIVATE -fno-pic -static)
    add_vk_library_to_no_pic_namespace(${ARGV0})
endfunction()

function(prepend VAR_NAME PREFIX)
    list(TRANSFORM ARGN PREPEND ${PREFIX} OUTPUT_VARIABLE ${VAR_NAME})
    set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)
endfunction(prepend)

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

function(detect_xcode_sdk_path SDK_PATH INCLUDE_DIRS)
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
            set(${SDK_PATH} "${sdk_path}" PARENT_SCOPE)
            message(STATUS "Detected Xcode SDK path: ${sdk_path}")
        else()
            message(FATAL_ERROR "Failed to detect Xcode SDK path: Output is empty.")
        endif()
    else()
        message(FATAL_ERROR "Failed to detect Xcode SDK path: ${stderr}")
    endif()

    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(${INCLUDE_DIRS} /usr/local/include)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(${INCLUDE_DIRS} /opt/homebrew/include)
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
                DEPENDS ${TARGET}
                COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${COMBINED_TARGET}.ar
                OUTPUT ${combined_target_path}
                COMMENT "Combining ${COMBINED_TARGET}"
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
                DEPENDS ${TARGET}
                COMMAND ${ar_tool} -static -o ${combined_target_path} "$<GENEX_EVAL:$<JOIN:${dependencies_list},$<SEMICOLON>>>"
                COMMAND_EXPAND_LISTS
                OUTPUT ${combined_target_path}
                COMMENT "Combining ${COMBINED_TARGET}"
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

function(get_submodule_remote_url SUBMODULE_PATH RESULT_VAR)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} config --file ${CMAKE_SOURCE_DIR}/.gitmodules --get submodule.${SUBMODULE_PATH}.url
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE submodule_url
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${RESULT_VAR} "${submodule_url}" PARENT_SCOPE)
endfunction()

function(get_submodule_version SUBMODULE_PATH RESULT_VAR)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} -C ${SUBMODULE_PATH} describe --tags
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE submodule_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    set(${RESULT_VAR} "${submodule_version}" PARENT_SCOPE)
endfunction()

function(make_third_party_configuration PIC_ENABLED PROJECT_GENERIC_NAME PROJECT_GENERIC_NAMESPACE OUT_PROJECT_NAME OUT_TARGET_NAME OUT_EXTRA_COMPILE_FLAGS OUT_NAMESPACE OUT_LIB_SPECIFIER)
    if(PIC_ENABLED)
        set(${OUT_PROJECT_NAME} "${PROJECT_GENERIC_NAME}${PIC_LIBRARY_SUFFIX}" PARENT_SCOPE)
        set(${OUT_TARGET_NAME} ${PROJECT_GENERIC_NAMESPACE}::${PIC_NAMESPACE}::${PROJECT_GENERIC_NAME} PARENT_SCOPE)
        if(APPLE)
            set(${OUT_EXTRA_COMPILE_FLAGS} "-fPIC --sysroot ${CMAKE_OSX_SYSROOT}" PARENT_SCOPE)
        else()
            set(${OUT_EXTRA_COMPILE_FLAGS} "-fPIC" PARENT_SCOPE)
        endif()
        set(${OUT_NAMESPACE} ${PIC_NAMESPACE} PARENT_SCOPE)
        set(${OUT_LIB_SPECIFIER} ${PIC_LIBRARY_SPECIFIER} PARENT_SCOPE)
    else()
        set(${OUT_PROJECT_NAME} "${PROJECT_GENERIC_NAME}${NO_PIC_LIBRARY_SUFFIX}" PARENT_SCOPE)
        set(${OUT_TARGET_NAME} ${PROJECT_GENERIC_NAMESPACE}::${NO_PIC_NAMESPACE}::${PROJECT_GENERIC_NAME} PARENT_SCOPE)
        if(APPLE)
            set(${OUT_EXTRA_COMPILE_FLAGS} "-fno-pic --sysroot ${CMAKE_OSX_SYSROOT}" PARENT_SCOPE)
        else()
            set(${OUT_EXTRA_COMPILE_FLAGS} "-fno-pic -static" PARENT_SCOPE)
        endif()
        set(${OUT_NAMESPACE} ${NO_PIC_NAMESPACE} PARENT_SCOPE)
        set(${OUT_LIB_SPECIFIER} ${NO_PIC_LIBRARY_SPECIFIER} PARENT_SCOPE)
    endif()
endfunction()

function(check_python_package PACKAGE_NAME)
    execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import ${PACKAGE_NAME}"
            RESULT_VARIABLE return_code
            OUTPUT_QUIET
            ERROR_QUIET
    )
    if(return_code EQUAL 0)
        message(STATUS "Found Python3 package `${PACKAGE_NAME}`")
    else()
        message(FATAL_ERROR "Python3 package `${PACKAGE_NAME}` is required but was not found")
    endif()
endfunction()