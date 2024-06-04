prepend(KPHP_CORE_RUNTIME_UTILS ${BASE_DIR}/runtime-core/utils/
        migration-php8.cpp
)

prepend(KPHP_CORE_KPHP_TYPES ${BASE_DIR}/runtime-core/kphp-types/definition/
        mixed.cpp
        string.cpp
        string_buffer.cpp
)

prepend(KPHP_CORE_MEMORY_RECOURCE ${BASE_DIR}/runtime-core/memory-resource/
        details/memory_chunk_tree.cpp
        details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp
)

set(KPHP_CORE_SRC
        ${KPHP_CORE_RUNTIME_UTILS}
        ${KPHP_CORE_KPHP_TYPES}
        ${KPHP_CORE_MEMORY_RECOURCE}
)

vk_add_library(runtime-core OBJECT ${KPHP_CORE_SRC})
