prepend(RUNTIME_KPHP_TYPES_SRC core/kphp_types/definition/
        string.cpp
        string_buffer.cpp
        mixed.cpp)

prepend(RUNTIME_LANGUAGE_SRC core/globals/
        php-script-globals.cpp)

prepend(RUNTIME_TYPES_SRC core/runtime_types/
        string_cache.cpp
        storage.cpp)

set(RUNTIME_CORE_SRC ${RUNTIME_TYPES_SRC}
        ${RUNTIME_KPHP_TYPES_SRC})