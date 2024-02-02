cmake_minimum_required(VERSION 3.16)
project(kphplight)

include(CMakePrintHelpers)
include(CheckCXXCompilerFlag)
include(AddFileDependencies)
include(FetchContent)

include(cmake/utils.cmake)
include(cmake/init-compilation-flags.cmake)

include_directories(.)


set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(BIN_DIR "${BASE_DIR}/objs")


include(${BASE_DIR}/script/script.cmake)
include(${BASE_DIR}/runtime/runtime.cmake)
include(${BASE_DIR}/platform/platform.cmake)
include(${BASE_DIR}/common/common.cmake)

add_library(kphplight SHARED $<TARGET_OBJECTS:script_src> $<TARGET_OBJECTS:runtime_src> $<TARGET_OBJECTS:common_src>)
set_target_properties(kphplight PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs)


target_link_libraries(platform PRIVATE kphplight $<TARGET_OBJECTS:utils_src>)
target_link_libraries(test PRIVATE kphplight GTest::gtest_main $<TARGET_OBJECTS:utils_src>)
