prepend(RUNTIME_CORE_UTILS runtime-core/utils/ migration-php8.cpp)

prepend(RUNTIME_CORE_TYPES runtime-core/core-types/definition/ mixed.cpp
        string.cpp string_buffer.cpp string_cache.cpp)

prepend(RUNTIME_CORE_MEMORY_RESOURCE runtime-core/memory-resource/
        details/memory_chunk_tree.cpp details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp unsynchronized_pool_resource.cpp)

set(RUNTIME_CORE_SRC ${RUNTIME_CORE_UTILS} ${RUNTIME_CORE_TYPES}
                     ${RUNTIME_CORE_MEMORY_RESOURCE})
