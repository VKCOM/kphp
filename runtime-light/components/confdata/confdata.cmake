# k2-confdata image: a standalone C++ component that shares the common
# runtime-light machinery but provides its own entry points and bindings
set(K2_CONFDATA_COMPONENT_SRC
    ${RUNTIME_LIGHT_DIR}/components/confdata/confdata-component.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/bindings/bindings.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/state/component-state.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/state/instance-state.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/state/predefined-wildcards-builder.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/confdata/confdata-keys.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/confdata/predefined-wildcards.cpp)

set(K2_CONFDATA_TL_SRC
    ${RUNTIME_LIGHT_DIR}/tl/tl-types.cpp
    ${RUNTIME_LIGHT_DIR}/tl/tl-functions.cpp)

set(K2_CONFDATA_ALLOCATOR_SRC
    ${RUNTIME_LIGHT_DIR}/allocator/runtime-light-allocator.cpp
    ${RUNTIME_LIGHT_DIR}/memory-resource-impl/monotonic-light-buffer-resource.cpp)

set(K2_CONFDATA_DIAGNOSTICS_SRC
    ${RUNTIME_LIGHT_DIR}/stdlib/diagnostics/backtrace.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/diagnostics/php-assert.cpp)

set(K2_CONFDATA_MEMORY_RESOURCE_SRC
    ${RUNTIME_COMMON_DIR}/core/memory-resource/unsynchronized_pool_resource.cpp
    ${RUNTIME_COMMON_DIR}/core/memory-resource/monotonic_buffer_resource.cpp
    ${RUNTIME_COMMON_DIR}/core/memory-resource/details/memory_chunk_tree.cpp
    ${RUNTIME_COMMON_DIR}/core/memory-resource/details/memory_ordered_chunk_list.cpp)

set(K2_CONFDATA_SRC
    ${K2_CONFDATA_COMPONENT_SRC}
    ${K2_CONFDATA_TL_SRC}
    ${K2_CONFDATA_ALLOCATOR_SRC}
    ${K2_CONFDATA_DIAGNOSTICS_SRC}
    ${K2_CONFDATA_MEMORY_RESOURCE_SRC}
    # link the alloc-wrapper objects directly (not as an archive) so that
    # __wrap_* definitions are always present regardless of link order
    $<TARGET_OBJECTS:libc-alloc-wrapper-pic>)

vk_add_library_pic(k2-confdata-pic SHARED ${K2_CONFDATA_SRC})
set_target_properties(k2-confdata-pic PROPERTIES PREFIX "" OUTPUT_NAME "k2-confdata" LIBRARY_OUTPUT_DIRECTORY ${OBJS_DIR})
target_compile_options(k2-confdata-pic PUBLIC ${RUNTIME_LIGHT_COMPILE_FLAGS})
# reuse the common link flags; cmake drives the link through the compiler,
# so bare ld options need the -Wl, prefix
set(K2_CONFDATA_LINK_FLAGS ${RUNTIME_LIGHT_LINK_FLAGS})
if(NOT APPLE)
  list(TRANSFORM K2_CONFDATA_LINK_FLAGS REPLACE "^--" "-Wl,--")
endif()
target_link_options(k2-confdata-pic PUBLIC ${K2_CONFDATA_LINK_FLAGS})

string(TIMESTAMP K2_CONFDATA_BUILD_TIMESTAMP "%s" UTC)
target_compile_definitions(k2-confdata-pic PRIVATE K2_CONFDATA_BUILD_TIMESTAMP=${K2_CONFDATA_BUILD_TIMESTAMP}ULL
                                                   K2_CONFDATA_COMPILER_VERSION="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
