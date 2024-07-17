prepend(KPHP_CORE_RUNTIME_UTILS ${BASE_DIR}/runtime-core/utils/
        migration-php8.cpp
)

prepend(KPHP_CORE_TYPES ${BASE_DIR}/runtime-core/core-types/definition/
        mixed.cpp
        string.cpp
        string_buffer.cpp
        string_cache.cpp
)

prepend(KPHP_CORE_MEMORY_RESOURCE ${BASE_DIR}/runtime-core/memory-resource/
        details/memory_chunk_tree.cpp
        details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp
)

set(KPHP_CORE_SRC
        ${KPHP_CORE_RUNTIME_UTILS}
        ${KPHP_CORE_TYPES}
        ${KPHP_CORE_MEMORY_RESOURCE}
)

vk_add_library(runtime-core OBJECT ${KPHP_CORE_SRC})
set_property(TARGET runtime-core PROPERTY POSITION_INDEPENDENT_CODE ON)