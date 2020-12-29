option(DOWNLOAD_MISSING_LIBRARIES "download and build missing libraries if needed" OFF)
cmake_print_variables(DOWNLOAD_MISSING_LIBRARIES)
function(handle_missing_library LIB_NAME)
    message(STATUS "------${LIB_NAME}---------")
    if(DOWNLOAD_MISSING_LIBRARIES)
        message(STATUS "${LIB_NAME} library is not found and will be downloaded automatically")
    else()
        message(FATAL_ERROR "${LIB_NAME} library is not found. Install it or pass -DDOWNLOAD_MISSING_LIBRARIES=On during cmake generation")
    endif()
endfunction()

find_package(fmt QUIET)
if(NOT fmt_FOUND)
    handle_missing_library("fmtlib")
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt
            GIT_TAG        7.0.3
    )
    FetchContent_MakeAvailable(fmt)
    include_directories(${fmt_SOURCE_DIR}/include)
    message(STATUS "---------------------")
endif()

set(H3_LIB h3)
set(H3_COMPILE_OPTIONS -Wno-float-conversion)
if (NOT APPLE)
    set(H3_LIB uber-${H3_LIB})
    set(H3_COMPILE_OPTIONS ${H3_COMPILE_OPTIONS} -Wno-maybe-uninitialized)
endif()
find_package(${H3_LIB} QUIET)
if(NOT ${H3_LIB}_FOUND)
    handle_missing_library("uber-h3")
    FetchContent_Declare(
            uber-h3
            GIT_REPOSITORY https://github.com/VKCOM/uber-h3
            GIT_TAG        dpkg-build
    )
    set(ENABLE_DOCS OFF CACHE BOOL "Disable docs for uber-h3.")
    set(ENABLE_LINTING OFF CACHE BOOL "Disable linting for uber-h3.")
    set(ENABLE_FORMAT OFF CACHE BOOL "Disable formatting for uber-h3.")
    set(ENABLE_COVERAGE OFF CACHE BOOL "Disable compiling tests with coverage for uber-h3.")
    set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable benchmarking applications for uber-h3.")
    set(BUILD_FILTERS OFF CACHE BOOL "Disable filter applications for uber-h3.")
    set(BUILD_GENERATORS OFF CACHE BOOL "Disable code generation applications for uber-h3.")

    FetchContent_MakeAvailable(uber-h3)
    target_compile_options(h3 PRIVATE ${H3_COMPILE_OPTIONS})
    add_library(uber-h3::h3 ALIAS h3)
    file(COPY ${uber-h3_BINARY_DIR}/src/h3lib/include/h3api.h DESTINATION ${uber-h3_BINARY_DIR}/include/h3)
    include_directories(${uber-h3_BINARY_DIR}/include)
    message(STATUS "---------------------")
endif()

if(KPHP_TESTS)
    find_package(GTest QUIET)

    if(GTest_FOUND)
        include(GoogleTest)
    else()
        handle_missing_library("gtest")
        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG        release-1.10.0
        )
        FetchContent_MakeAvailable(googletest)
        add_library(GTest::GTest ALIAS gtest)
        add_library(GTest::Main ALIAS gtest_main)
        message(STATUS "---------------------")
    endif()
endif()

if(APPLE)
    if (DEFINED ENV{EPOLL_SHIM_REPO})
        FetchContent_Declare(
                epoll
                URL $ENV{EPOLL_SHIM_REPO}
        )
    else()
        handle_missing_library("epoll-shim")
        FetchContent_Declare(
                epoll
                GIT_REPOSITORY https://github.com/VKCOM/epoll-shim
                GIT_TAG osx-platform
        )
        message(STATUS "---------------------")
    endif()
    FetchContent_MakeAvailable(epoll)
    include_directories(${epoll_SOURCE_DIR}/include)
    add_definitions(-DEPOLL_SHIM_LIB_DIR="${epoll_BINARY_DIR}/src")
    set(EPOLL_SHIM_LIB epoll-shim)
endif()
