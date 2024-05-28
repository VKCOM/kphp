prepend(KPHP_CORE_RUNTIME_TYPES ${BASE_DIR}/kphp-core/runtime-types/
        string_cache.cpp
)

prepend(KPHP_CORE_RUNTIME_FUNTCIONS ${BASE_DIR}/kphp-core/functions/
        migration_php8.cpp
)

prepend(KPHP_CORE_KPHP_TYPES ${BASE_DIR}/kphp-core/kphp-types/definition/
        mixed.cpp
        string.cpp
        string_buffer.cpp
)

prepend(KPHP_CORE_MEMORY_RECOURCE ${BASE_DIR}/kphp-core/memory_resource/
        details/memory_chunk_tree.cpp
        details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp
)

set(KPHP_CORE_SRC
        ${KPHP_CORE_RUNTIME_FUNTCIONS}
        ${KPHP_CORE_RUNTIME_TYPES}
        ${KPHP_CORE_KPHP_TYPES}
        ${KPHP_CORE_MEMORY_RECOURCE}
)

vk_add_library(kphp_core OBJECT ${KPHP_CORE_SRC})
