function(apply_patches_from_series build_dir series_file patch_dir)
    # Check for the presence of the patch utility
    find_program(PATCH_EXECUTABLE patch)
    if(NOT PATCH_EXECUTABLE)
        message(FATAL_ERROR "The 'patch' utility is not found on this system.")
    endif()

    # Read the series file and apply each patch listed
    file(READ "${series_file}" series_content)
    string(REPLACE "\n" ";" series_list "${series_content}")

    foreach(patch IN LISTS series_list)
        if(NOT patch STREQUAL "")
            # Construct the full path to the patch file
            set(patch_file "${patch_dir}${patch}")

            # Apply the patch using GNU patch
            execute_process(
                    COMMAND ${PATCH_EXECUTABLE} -p1 -i "${patch_file}"
                    WORKING_DIRECTORY "${build_dir}"
                    RESULT_VARIABLE result
                    OUTPUT_VARIABLE output
                    ERROR_VARIABLE error
            )

            if(NOT result EQUAL 0)
                message(FATAL_ERROR "Failed to apply patch: ${patch}\nOutput: ${output}\nError: ${error}")
            else()
                message(STATUS "Applied patch: ${patch}")
            endif()
        endif()
    endforeach()
endfunction()

apply_patches_from_series(${BUILD_DIR} ${PATCH_SERIES} ${PATCH_DIR})
