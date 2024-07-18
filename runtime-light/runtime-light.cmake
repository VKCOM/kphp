include(${BASE_DIR}/runtime-light/allocator/allocator.cmake)
include(${BASE_DIR}/runtime-light/core/core.cmake)
include(${BASE_DIR}/runtime-light/stdlib/stdlib.cmake)
include(${BASE_DIR}/runtime-light/streams/streams.cmake)
include(${BASE_DIR}/runtime-light/tl/tl.cmake)
include(${BASE_DIR}/runtime-light/utils/utils.cmake)

prepend(MONOTOINC_LIGHT_BUFFER_RESOURCE_SRC ${BASE_DIR}/runtime-light/memory-resource-impl/
        monotonic-light-buffer-resource.cpp)

prepend(RUNTIME_COMPONENT_SRC ${BASE_DIR}/runtime-light/
        component/component.cpp)

set(RUNTIME_LIGHT_SRC ${RUNTIME_CORE_SRC}
        ${RUNTIME_STDLIB_SRC}
        ${RUNTIME_ALLOCATOR_SRC}
        ${RUNTIME_COROUTINE_SRC}
        ${RUNTIME_COMPONENT_SRC}
        ${RUNTIME_STREAMS_SRC}
        ${RUNTIME_TL_SRC}
        ${RUNTIME_UTILS_SRC}
        ${RUNTIME_LANGUAGE_SRC}
        ${MONOTOINC_LIGHT_BUFFER_RESOURCE_SRC}
        ${BASE_DIR}/runtime-light/runtime-light.cpp)

vk_add_library(runtime-light OBJECT ${RUNTIME_LIGHT_SRC})
set_property(TARGET runtime-light PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(runtime-light PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs)

vk_add_library(kphp-light-runtime STATIC)
target_link_libraries(kphp-light-runtime PUBLIC vk::light_common vk::runtime-light vk::runtime-core)
set_target_properties(kphp-light-runtime PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OBJS_DIR})

file(GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
        RELATIVE ${BASE_DIR}
        CONFIGURE_DEPENDS
        "${BASE_DIR}/runtime-light/*.h")
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(WRITE ${AUTO_DIR}/runtime/runtime-headers.h "\
#ifndef MERGED_RUNTIME_LIGHT_HEADERS_H
#define MERGED_RUNTIME_LIGHT_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp
        [[
#include "auto/runtime/runtime-headers.h"
]])

add_library(php_lib_version_j OBJECT ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp)
target_compile_options(php_lib_version_j PRIVATE -I. -E)
add_dependencies(php_lib_version_j kphp-light-runtime)

add_custom_command(OUTPUT ${OBJS_DIR}/php_lib_version.sha256
        COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
        DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
        COMMENT "php_lib_version.sha256 generation")
