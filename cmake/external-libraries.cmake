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

if(KPHP_TESTS)
    find_package(GTest QUIET)

    if(GTest_FOUND)
        include(GoogleTest)
        # Check if gmock is not available
        if(NOT TARGET GTest::gmock)
            find_library(GMOCK_LIBRARY
                    NAMES gmock
                    HINTS ${CMAKE_INSTALL_FULL_LIBDIR} ${CMAKE_OSX_SYSROOT}
            )
            if(GMOCK_LIBRARY)
                include(GNUInstallDirs)
                message(STATUS "Found Google Mock library: ${GMOCK_LIBRARY}")
                add_library(gmock STATIC IMPORTED)
                set_target_properties(gmock PROPERTIES
                        IMPORTED_LOCATION ${GMOCK_LIBRARY}
                        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_FULL_INCLUDEDIR};${CMAKE_OSX_INCLUDE_DIRS}"
                )
                add_library(GTest::gmock ALIAS gmock)
            else()
                message(FATAL_ERROR "Google Mock library not found. Please install libgmock-dev.")
            endif()
        else()
            message(STATUS "Google Mock found in GTest")
        endif()
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
