prepend(MEMORY_RESOURSE_DETAILS_SRC ${BASE_DIR}/runtime-light/allocator/memory_resource/details/
        memory_chunk_tree.cpp
        memory_ordered_chunk_list.cpp)

prepend(MEMORY_RESOURSE_SRC ${BASE_DIR}/runtime-light/allocator/memory_resource/
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp)

set(RUNTIME_ALLOCATOR_SRC ${MEMORY_RESOURSE_DETAILS_SRC}
                          ${MEMORY_RESOURSE_SRC}
                          ${BASE_DIR}/runtime-light/allocator/allocator.cpp)
