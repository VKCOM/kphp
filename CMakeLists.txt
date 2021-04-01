cmake_minimum_required(VERSION 3.14..3.18)
project(kphp
        VERSION 1.0.1
        DESCRIPTION "Compiler for PHP (aka KPHP)"
        HOMEPAGE_URL https://github.com/VKCOM/kphp)

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR [[
        In-source build is forbidden! Run following command:
        rm -rf CMakeFiles CMakeCache.txt && cmake -H. -Bbuild
    ]])
endif()

# include(CMakeGraphVizOptions)
# set(GRAPHVIZ_CUSTOM_TARGETS TRUE)

include(CMakePrintHelpers)
include(CheckCXXCompilerFlag)
include(AddFileDependencies)
include(FetchContent)

# Global includes must be before all other includes/add_subdirectories
include(cmake/utils.cmake)
include(cmake/init-global-vars.cmake)
include(cmake/init-compilation-flags.cmake)
include(cmake/popular-common.cmake)
include(cmake/external-libraries.cmake)

# TODO: use FetchContent_Declare instead of include_directories
include_directories(third_party/optional/include/tl)
include_directories(.)

# Custom modules
include(${BASE_DIR}/flex/flex.cmake)
include(${BASE_DIR}/net/net.cmake)
include(${BASE_DIR}/vkext/vkext.cmake)

include(${COMMON_DIR}/binlog/binlog.cmake)
include(${COMMON_DIR}/common.cmake)
include(${COMMON_DIR}/tl/tl.cmake)
include(${COMMON_DIR}/unicode/unicode.cmake)

include(${BASE_DIR}/runtime/runtime.cmake)
include(${BASE_DIR}/server/server.cmake)
include(${BASE_DIR}/compiler/compiler.cmake)

include(${BASE_DIR}/tests/tests.cmake)

add_custom_target(kphp ALL DEPENDS ${OBJS_DIR}/php_lib_version.sha256)
add_dependencies(kphp kphp2cpp kphp-full-runtime)

install(TARGETS kphp2cpp
        COMPONENT KPHP
        RUNTIME DESTINATION ${VK_INSTALL_DIR}/bin/)
install_symlink(${VK_INSTALL_DIR}/bin/kphp2cpp bin/kphp KPHP)

install(TARGETS kphp-full-runtime
        COMPONENT KPHP
        LIBRARY DESTINATION ${INSTALL_KPHP_SOURCE}/objs
        ARCHIVE DESTINATION ${INSTALL_KPHP_SOURCE}/objs)

install(DIRECTORY ${COMMON_DIR}
                  ${BASE_DIR}/runtime
                  ${BASE_DIR}/server
        COMPONENT KPHP
        DESTINATION ${INSTALL_KPHP_SOURCE}
        FILES_MATCHING REGEX ".*\\.(h|inl)$")

install(FILES ${OBJS_DIR}/php_lib_version.sha256
        COMPONENT KPHP
        DESTINATION ${INSTALL_KPHP_SOURCE}/objs)

install(FILES ${BASE_DIR}/functions.txt ${COMMON_DIR}/php-functions.h
        COMPONENT KPHP
        DESTINATION ${INSTALL_KPHP_SOURCE}/)

install(FILES ${AUTO_DIR}/runtime/runtime-headers.h
        COMPONENT KPHP
        DESTINATION ${INSTALL_KPHP_SOURCE}/objs/generated/auto/runtime/)

set(CPACK_DEBIAN_KPHP_PACKAGE_DEPENDS "vk-flex-data, curl-kphp-vk, libuber-h3, libpcre3-dev, libre2-dev, libyaml-cpp-dev, libssl-dev, zlib1g-dev, \
                                       libzstd-dev, g++, libnghttp2-dev, libmsgpack-dev, kphp-timelib")
set(CPACK_DEBIAN_KPHP_PACKAGE_RECOMMENDS "php7.4-vkext, vk-tl-tools")
set(CPACK_DEBIAN_KPHP_DESCRIPTION "kphp2cpp compiler and runtime for it")
set(CPACK_DEBIAN_KPHP_PACKAGE_NAME "kphp")

add_custom_target(force_clean_all
                  COMMAND rm -rf ${OBJS_DIR}
                  COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/*)

include(CPack)
