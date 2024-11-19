prepend(CORE_UTILS core/utils/ migration-php8.cpp)

prepend(CORE_TYPES core/core-types/definition/ mixed.cpp string.cpp
        string_buffer.cpp string_cache.cpp)

prepend(CORE_MEMORY_RESOURCE core/memory-resource/
        details/memory_chunk_tree.cpp details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp unsynchronized_pool_resource.cpp)

prepend(CORE_CONTEXT core/
        array_access_context.cpp)

set(CORE_SRC ${CORE_UTILS} ${CORE_TYPES} ${CORE_MEMORY_RESOURCE} ${CORE_CONTEXT})
