include(${RUNTIME_LIGHT_DIR}/allocator/allocator.cmake)
include(${RUNTIME_LIGHT_DIR}/core/core.cmake)
include(${RUNTIME_LIGHT_DIR}/scheduler/scheduler.cmake)
include(${RUNTIME_LIGHT_DIR}/stdlib/stdlib.cmake)
include(${RUNTIME_LIGHT_DIR}/streams/streams.cmake)
include(${RUNTIME_LIGHT_DIR}/tl/tl.cmake)
include(${RUNTIME_LIGHT_DIR}/utils/utils.cmake)
include(${RUNTIME_LIGHT_DIR}/component/component.cmake)
include(${RUNTIME_LIGHT_DIR}/memory-resource-impl/memory-resource-impl.cmake)

set(RUNTIME_LIGHT_SRC
    ${RUNTIME_CORE_SRC}
    ${RUNTIME_STDLIB_SRC}
    ${RUNTIME_SCHEDULER_SRC}
    ${RUNTIME_ALLOCATOR_SRC}
    ${RUNTIME_COROUTINE_SRC}
    ${RUNTIME_COMPONENT_SRC}
    ${RUNTIME_STREAMS_SRC}
    ${RUNTIME_TL_SRC}
    ${RUNTIME_UTILS_SRC}
    ${RUNTIME_LANGUAGE_SRC}
    ${RUNTIME_MEMORY_RESOURCE_IMPL_SRC}
    runtime-light.cpp)

set(RUNTIME_SOURCES_FOR_COMP "${RUNTIME_LIGHT_SRC}")
configure_file(${BASE_DIR}/compiler/runtime_sources.h.in
               ${AUTO_DIR}/compiler/runtime_sources.h)

prepend(RUNTIME_LIGHT_SRC ${RUNTIME_LIGHT_DIR}/ "${RUNTIME_LIGHT_SRC}")

vk_add_library(runtime-light OBJECT ${RUNTIME_LIGHT_SRC})
set_property(TARGET runtime-light PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(runtime-light PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                               ${BASE_DIR}/objs)
target_compile_options(runtime-light PUBLIC -stdlib=libc++)
target_link_options(runtime-light PUBLIC -stdlib=libc++ -static-libstdc++)
target_compile_options(runtime-light PUBLIC -fPIC)

vk_add_library(kphp-light-runtime STATIC)
target_link_libraries(
  kphp-light-runtime PUBLIC vk::light_common vk::runtime-light vk::runtime-core)
set_target_properties(kphp-light-runtime PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
                                                    ${OBJS_DIR})

file(
  GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
  RELATIVE ${BASE_DIR}
  CONFIGURE_DEPENDS "${RUNTIME_LIGHT_DIR}/*.h")
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(
  WRITE ${AUTO_DIR}/runtime/runtime-headers.h
  "\
#ifndef MERGED_RUNTIME_LIGHT_HEADERS_H
#define MERGED_RUNTIME_LIGHT_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")

file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp
  [[
#include "auto/runtime/runtime-headers.h"
]])

add_library(php_lib_version_j OBJECT
            ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp)
target_compile_options(php_lib_version_j PRIVATE -I. -E)
add_dependencies(php_lib_version_j kphp-light-runtime)

add_custom_command(
  OUTPUT ${OBJS_DIR}/php_lib_version.sha256
  COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk
          '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
  DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
  COMMENT "php_lib_version.sha256 generation")

add_custom_target(php_lib_version_sha_256
                  DEPENDS ${OBJS_DIR}/php_lib_version.sha256)

# Light runtime uses C++20 coroutines heavily, so they are required
if(COMPILE_RUNTIME_LIGHT)
    get_property(TRY_COMPILE_COMPILE_OPTIONS TARGET runtime-light PROPERTY COMPILE_OPTIONS)
    string (REPLACE ";" " " TRY_COMPILE_COMPILE_OPTIONS "${TRY_COMPILE_COMPILE_OPTIONS}")
    file(WRITE "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp"
            "#include<coroutine>\n"
            "int main() {}\n")
    try_compile(
            HAS_COROUTINE
            "${PROJECT_BINARY_DIR}/tmp"
            "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp"
            COMPILE_DEFINITIONS "${TRY_COMPILE_COMPILE_OPTIONS}"
    )
    if(NOT HAS_COROUTINE)
        message(FATAL_ERROR "Compiler or libstdc++ does not support coroutines")
    endif()
    file(REMOVE "${PROJECT_BINARY_DIR}/check_coroutine_include.cpp")
endif()

get_property(
  RUNTIME_COMPILE_FLAGS
  TARGET runtime-light
  PROPERTY COMPILE_OPTIONS)
get_property(
  RUNTIME_INCLUDE_DIRS
  TARGET runtime-light
  PROPERTY INCLUDE_DIRECTORIES)

list(JOIN RUNTIME_COMPILE_FLAGS "\;" RUNTIME_COMPILE_FLAGS)
string(REPLACE "\"" "\\\"" RUNTIME_COMPILE_FLAGS ${RUNTIME_COMPILE_FLAGS})
configure_file(${BASE_DIR}/compiler/runtime_compile_flags.h.in
               ${AUTO_DIR}/compiler/runtime_compile_flags.h)
