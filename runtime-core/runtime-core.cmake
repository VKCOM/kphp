prepend(KPHP_CORE_RUNTIME_UTILS utils/
        migration-php8.cpp
)

prepend(KPHP_CORE_TYPES core-types/definition/
        mixed.cpp
        string.cpp
        string_buffer.cpp
        string_cache.cpp
)

prepend(KPHP_CORE_MEMORY_RESOURCE memory-resource/
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

if (COMPILE_RUNTIME_LIGHT)
    set(RUNTIME_CORE_SOURCES_FOR_COMP "${KPHP_CORE_SRC}")
    configure_file(${BASE_DIR}/compiler/runtime_core_sources.h.in ${AUTO_DIR}/compiler/runtime_core_sources.h)
endif()

prepend(KPHP_CORE_SRC ${RUNTIME_CORE_DIR}/ "${KPHP_CORE_SRC}")
vk_add_library(runtime-core OBJECT ${KPHP_CORE_SRC})
