prepend(RUNTIME_TYPES_SRC ${BASE_DIR}/runtime/kphp_types/definition/
        string.cpp
        string_buffer.cpp)

prepend(RUNTIME_COMMON_SRC ${BASE_DIR}/runtime/
        php_assert.cpp)

set(RUNTIME_SRC ${RUNTIME_TYPES_SRC}
        ${RUNTIME_COMMON_SRC})

add_library(runtime_src OBJECT ${RUNTIME_SRC})
set_property(TARGET runtime_src PROPERTY POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
        RELATIVE ${BASE_DIR}
        CONFIGURE_DEPENDS
        "${BASE_DIR}/runtime/*.h")
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(WRITE ${BIN_DIR}/runtime/runtime-headers.h "\
#ifndef MERGED_RUNTIME_HEADERS_H
#define MERGED_RUNTIME_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")
