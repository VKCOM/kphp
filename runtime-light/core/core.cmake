prepend(RUNTIME_KPHP_TYPES_SRC ${BASE_DIR}/runtime-light/core/kphp_types/definition/
        string.cpp
        string_buffer.cpp
        mixed.cpp)

prepend(RUNTIME_TYPES_SRC ${BASE_DIR}/runtime-light/core/runtime_types/
        string_cache.cpp)

set(RUNTIME_CORE_SRC ${RUNTIME_TYPES_SRC}
        ${RUNTIME_KPHP_TYPES_SRC})