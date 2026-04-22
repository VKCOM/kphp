prepend(CORE_UTILS core/utils/ migration-php8.cpp)

prepend(CORE_TYPES core/core-types/definition/ mixed.cpp string.cpp
        string_buffer.cpp string_cache.cpp)

prepend(CORE_MEMORY_RESOURCE core/memory-resource/
        details/memory_chunk_tree.cpp details/memory_ordered_chunk_list.cpp
        monotonic_buffer_resource.cpp unsynchronized_pool_resource.cpp)


set(SOURCES
        runtime-common/core/memory-resource/details/memory_chunk_tree.cpp
        runtime-common/core/memory-resource/details/memory_ordered_chunk_list.cpp
        runtime-common/core/memory-resource/monotonic_buffer_resource.cpp
        runtime-common/core/memory-resource/unsynchronized_pool_resource.cpp
        runtime-common/core/allocator_test.cpp
)

add_executable(allocator_test ${SOURCES})
target_include_directories(allocator_test PRIVATE runtime-common/core/memory-resource)

target_compile_options(allocator_test PRIVATE -O3)
set(CORE_SRC ${CORE_UTILS} ${CORE_TYPES} ${CORE_MEMORY_RESOURCE})
