option(DOWNLOAD_MISSING_LIBRARIES "download and build missing libraries if needed" OFF)
option(MBFL "download and build libmbfl" OFF)
cmake_print_variables(DOWNLOAD_MISSING_LIBRARIES)
cmake_print_variables(MBFL)
function(handle_missing_library LIB_NAME)
    message(STATUS "------${LIB_NAME}---------")
    if(DOWNLOAD_MISSING_LIBRARIES)
        message(STATUS "${LIB_NAME} library is not found and will be downloaded automatically")
    else()
        message(FATAL_ERROR "${LIB_NAME} library is not found. Install it or pass -DDOWNLOAD_MISSING_LIBRARIES=On during cmake generation")
    endif()
endfunction()

if(MBFL)
    message(STATUS "MBFL=On, libmbfl will be downloaded and built")
    add_compile_options(-DMBFL)
    FetchContent_Declare(libmbfl GIT_REPOSITORY https://github.com/andreylzmw/libmbfl)
    FetchContent_MakeAvailable(libmbfl)
    include_directories(${libmbfl_SOURCE_DIR}/include)
    add_definitions(-DLIBMBFL_LIB_DIR="${libmbfl_SOURCE_DIR}/objs")
    add_link_options(-L${libmbfl_SOURCE_DIR}/objs)

    find_package(onig QUIET)
    if(NOT onig_FOUND)
        handle_missing_library("onig")
        FetchContent_Declare(
                onig
                GIT_REPOSITORY https://github.com/kkos/oniguruma/
                GIT_TAG        v6.9.9
        )
        FetchContent_MakeAvailable(onig)
        include_directories(${onig_SOURCE_DIR}/src)
        message(STATUS "---------------------")
    endif()
endif()

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

find_library(KPHP_TIMELIB kphp-timelib)
if(KPHP_TIMELIB)
    add_library(kphp-timelib STATIC IMPORTED ${KPHP_TIMELIB})
else()
    handle_missing_library("kphp-timelib")
    FetchContent_Declare(kphp-timelib GIT_REPOSITORY https://github.com/VKCOM/timelib)
    message(STATUS "---------------------")
    FetchContent_MakeAvailable(kphp-timelib)
    include_directories(${kphp-timelib_SOURCE_DIR}/include)
    add_definitions(-DKPHP_TIMELIB_LIB_DIR="${kphp-timelib_SOURCE_DIR}/objs")
    add_link_options(-L${kphp-timelib_SOURCE_DIR}/objs)
endif()

find_library(KPHP_H3 h3)
if(KPHP_H3)
    add_library(libh3 STATIC IMPORTED ${KPHP_H3})
else()
    handle_missing_library("h3")
    FetchContent_Declare(
            h3
            GIT_REPOSITORY https://github.com/VKCOM/uber-h3.git
            GIT_TAG        fetch_content
    )
    message(STATUS "---------------------")
    FetchContent_MakeAvailable(h3)
    include_directories(${h3_BINARY_DIR}/src/include)
    add_definitions(-DKPHP_H3_LIB_DIR="${h3_BINARY_DIR}/lib")
    add_link_options(-L${h3_BINARY_DIR}/lib)
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
