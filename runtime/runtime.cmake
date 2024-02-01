prepend(RUNTIME_SRC ${BASE_DIR}/runtime/
        tmp.cpp)

add_library(runtime_src OBJECT ${RUNTIME_SRC})

target_include_directories(script_src PRIVATE ${BASE_DIR})

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
