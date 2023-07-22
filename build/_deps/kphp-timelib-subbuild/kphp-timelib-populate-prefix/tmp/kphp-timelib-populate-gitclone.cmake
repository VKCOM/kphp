
if(NOT "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitinfo.txt" IS_NEWER_THAN "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitclone-lastrun.txt")
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: '/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"  clone --config "advice.detachedHead=false" "https://github.com/VKCOM/timelib" "kphp-timelib-src"
    WORKING_DIRECTORY "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/VKCOM/timelib'")
endif()

execute_process(
  COMMAND "/usr/bin/git"  checkout master --
  WORKING_DIRECTORY "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'master'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git"  submodule update --recursive --init 
    WORKING_DIRECTORY "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src"
    RESULT_VARIABLE error_code
    )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitinfo.txt"
    "/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/opt/KasperskyOS-Community-Edition-1.1.1.13/examples/kphp/build/_deps/kphp-timelib-subbuild/kphp-timelib-populate-prefix/src/kphp-timelib-populate-stamp/kphp-timelib-populate-gitclone-lastrun.txt'")
endif()

