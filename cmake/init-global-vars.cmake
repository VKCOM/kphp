include_guard(GLOBAL)

set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(COMMON_DIR "${BASE_DIR}/common")
set(THIRD_PARTY_DIR "${BASE_DIR}/third-party")
set(OBJS_DIR ${BASE_DIR}/objs)
set(BIN_DIR ${OBJS_DIR}/bin)
set(LIB_DIR ${OBJS_DIR}/lib)
set(INCLUDE_DIR ${OBJS_DIR}/include)
set(GENERATED_DIR "${OBJS_DIR}/generated")
set(AUTO_DIR "${GENERATED_DIR}/auto")
set(RUNTIME_LIGHT_DIR "${BASE_DIR}/runtime-light")
set(RUNTIME_COMMON_DIR "${BASE_DIR}/runtime-common")

set(PIC_NAMESPACE "pic")
set(PIC_LIBRARY_SUFFIX "-${PIC_NAMESPACE}")
set(PIC_LIBRARY_SPECIFIER "PIC")

set(NO_PIC_NAMESPACE "no-pic")
set(NO_PIC_LIBRARY_SUFFIX "-${NO_PIC_NAMESPACE}")
set(NO_PIC_LIBRARY_SPECIFIER "NO_PIC")

if(APPLE)
    detect_xcode_sdk_path(CMAKE_OSX_SYSROOT CMAKE_OSX_INCLUDE_DIRS)
    set(ICONV_LIB iconv)
else()
    set(RT_LIB rt)
endif()

find_package(Git REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Perl REQUIRED)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

find_program(PHP_BIN php REQUIRED)

check_cxx_compiler_flag(-no-pie NO_PIE_IS_FOUND)
if(NO_PIE_IS_FOUND)
    set(PIE_MODE_FLAGS -no-pie)
    set(PIC_MODE_ENABLED ON)
    set(PIC_MODE ${NO_PIC_NAMESPACE})
else()
    set(PIE_MODE_FLAGS)
    set(PIC_MODE_ENABLED OFF)
    set(PIC_MODE ${PIC_NAMESPACE})
endif()

# add extra build type release without NDEBUG flag
set(DEFAULT_BUILD_TYPE "ReleaseWithAsserts")
set_property(CACHE CMAKE_BUILD_TYPE APPEND PROPERTY STRINGS ${DEFAULT_BUILD_TYPE})

# configure version-string
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY ${BASE_DIR}
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "refs/heads/" "" ${GIT_BRANCH} GIT_BRANCH)

execute_process(COMMAND ${GIT_EXECUTABLE} -c log.showSignature=false log -1 --pretty=format:%H
                WORKING_DIRECTORY ${BASE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND date +%s
                OUTPUT_VARIABLE BUILD_TIMESTAMP
                OUTPUT_STRIP_TRAILING_WHITESPACE)
cmake_print_variables(GIT_BRANCH GIT_COMMIT BUILD_TIMESTAMP)

set(VERSION_SUFFIX "")
if(NOT GIT_BRANCH STREQUAL "master")
    string(APPEND VERSION_SUFFIX "branch ${GIT_BRANCH}")
endif()

if(DEFINED ENV{BUILD_ID})
    string(APPEND VERSION_SUFFIX "build $ENV{BUILD_ID}")
endif()

set_property(SOURCE ${COMMON_DIR}/version-string.cpp
             APPEND
             PROPERTY COMPILE_DEFINITIONS
             COMMIT="${GIT_COMMIT} ${VERSION_SUFFIX}" BUILD_TIMESTAMP=${BUILD_TIMESTAMP})

set(VK_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/vkontakte)
set(INSTALL_KPHP_SOURCE ${VK_INSTALL_DIR}/kphp_source)

set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_COMPONENTS_GROUPING "IGNORE")
set(CPACK_GENERATOR "DEB")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "KPHP Team")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_PACKAGE_HOMEPAGE_URL ${CMAKE_PROJECT_HOMEPAGE_URL})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ".") # prevent error message from old cpack
#set(CPACK_RESOURCE_FILE_LICENSE "${BASE_DIR}/licence")

if(DEFINED ENV{PACKAGE_VERSION})
    set(CPACK_PACKAGE_VERSION $ENV{PACKAGE_VERSION})
    cmake_print_variables(CPACK_PACKAGE_VERSION)
    set(DEFAULT_KPHP_PATH ${INSTALL_KPHP_SOURCE})
else()
    set(DEFAULT_KPHP_PATH ${BASE_DIR})
endif()

# Ensure the dir for dependencies will be created
file(MAKE_DIRECTORY ${LIB_DIR})
file(MAKE_DIRECTORY ${INCLUDE_DIR})

cmake_print_variables(CMAKE_INSTALL_PREFIX CPACK_PACKAGING_INSTALL_PREFIX)
