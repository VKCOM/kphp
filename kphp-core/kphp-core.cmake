prepend(KPHP_CORE_RUNTIME_TYPES ${BASE_DIR}/kphp-core/runtime-types/
        string_cache.cpp
)

prepend(KPHP_CORE_RUNTIME_FUNTCIONS ${BASE_DIR}/kphp-core/funtcions/
        migration_php8.cpp
)

prepend(KPHP_CORE_KPHP_TYPES ${BASE_DIR}/kphp-core/kphp-types/definition/
        mixed.cpp
        string.cpp
        string_buffer.cpp
)

set(KPHP_CORE_SRC
        ${KPHP_CORE_RUNTIME_TYPES}
        ${KPHP_CORE_KPHP_TYPES}
)

vk_add_library(kphp_core OBJECT ${KPHP_CORE_SRC})
