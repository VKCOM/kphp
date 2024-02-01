prepend(COMMON_ALGORITHMS_SRC ${BASE_DIR}/common/algorithms/
        simd-int-to-string.cpp)

set(COMMON_ALL_SOURCES
        ${COMMON_ALGORITHMS_SRC})

add_library(common_src OBJECT ${COMMON_ALL_SOURCES})
set_property(TARGET common_src PROPERTY POSITION_INDEPENDENT_CODE ON)