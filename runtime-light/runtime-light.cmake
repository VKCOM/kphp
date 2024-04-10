include(${BASE_DIR}/runtime-light/allocator/allocator.cmake)

include(${BASE_DIR}/runtime-light/core/core.cmake)

include(${BASE_DIR}/runtime-light/stdlib/stdlib.cmake)

include(${BASE_DIR}/runtime-light/streams/streams.cmake)

include(${BASE_DIR}/runtime-light/utils/utils.cmake)

prepend(RUNTIME_COMPONENT_SRC ${BASE_DIR}/runtime-light/
        component/component.cpp)

set(RUNTIME_LIGHT_SRC ${RUNTIME_CORE_SRC}
                ${RUNTIME_STDLIB_SRC}
                ${RUNTIME_ALLOCATOR_SRC}
                ${RUNTIME_COROUTINE_SRC}
                ${RUNTIME_COMPONENT_SRC}
                ${RUNTIME_STREAMS_SRC}
                ${RUNTIME_UTILS_SRC}
                ${BASE_DIR}/runtime-light/runtime-light.cpp)

vk_add_library(runtimelight OBJECT ${RUNTIME_LIGHT_SRC})
set_property(TARGET runtimelight PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(runtimelight PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs)

file(GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
        RELATIVE ${BASE_DIR}
        CONFIGURE_DEPENDS
        "${BASE_DIR}/runtime-light/*.h")
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(WRITE ${BIN_DIR}/runtime/runtime-headers.h "\
#ifndef MERGED_RUNTIME_HEADERS_H
#define MERGED_RUNTIME_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")
