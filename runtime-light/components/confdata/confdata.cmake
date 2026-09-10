# kphp-confdata image: a standalone C++ component that shares the common
# runtime-light machinery but provides its own entry points and bindings
set(KPHP_CONFDATA_COMPONENT_SRC
    ${RUNTIME_LIGHT_DIR}/components/confdata/confdata-component.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/bindings/bindings.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/state/component-state.cpp
    ${RUNTIME_LIGHT_DIR}/components/confdata/state/instance-state.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/confdata/confdata-storage.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/confdata/confdata-keys.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/confdata/predefined-wildcards.cpp)

set(KPHP_CONFDATA_TL_SRC
    ${RUNTIME_LIGHT_DIR}/tl/tl-types.cpp
    ${RUNTIME_LIGHT_DIR}/tl/tl-functions.cpp)

set(KPHP_CONFDATA_ALLOCATOR_SRC ${RUNTIME_LIGHT_ALLOCATOR_SRC})
list(TRANSFORM KPHP_CONFDATA_ALLOCATOR_SRC PREPEND "${RUNTIME_LIGHT_DIR}/")
list(APPEND KPHP_CONFDATA_ALLOCATOR_SRC
    ${RUNTIME_LIGHT_DIR}/memory-resource-impl/monotonic-light-buffer-resource.cpp)

set(KPHP_CONFDATA_DIAGNOSTICS_SRC
    ${RUNTIME_LIGHT_DIR}/stdlib/diagnostics/backtrace.cpp
    ${RUNTIME_LIGHT_DIR}/stdlib/diagnostics/php-assert.cpp)

set(KPHP_CONFDATA_SERIALIZATION_SRC
    ${RUNTIME_COMMON_DIR}/stdlib/serialization/json-functions.cpp
    ${RUNTIME_COMMON_DIR}/stdlib/serialization/serialize-functions.cpp)

set(KPHP_CONFDATA_RUNTIME_CORE_SRC ${CORE_SRC})
list(TRANSFORM KPHP_CONFDATA_RUNTIME_CORE_SRC PREPEND "${RUNTIME_COMMON_DIR}/")

set(KPHP_CONFDATA_SRC
    ${KPHP_CONFDATA_COMPONENT_SRC}
    ${KPHP_CONFDATA_TL_SRC}
    ${KPHP_CONFDATA_ALLOCATOR_SRC}
    ${KPHP_CONFDATA_DIAGNOSTICS_SRC}
    ${KPHP_CONFDATA_SERIALIZATION_SRC}
    ${KPHP_CONFDATA_RUNTIME_CORE_SRC}
    # link the alloc-wrapper objects directly (not as an archive) so that
    # __wrap_* definitions are always present regardless of link order
    $<TARGET_OBJECTS:libc-alloc-wrapper-pic>)

vk_add_library_pic(kphp-confdata-pic SHARED ${KPHP_CONFDATA_SRC})
set_target_properties(kphp-confdata-pic PROPERTIES PREFIX "" OUTPUT_NAME "kphp-confdata" LIBRARY_OUTPUT_DIRECTORY ${OBJS_DIR})
target_compile_options(kphp-confdata-pic PUBLIC ${RUNTIME_LIGHT_COMPILE_FLAGS})
# reuse the common link flags; cmake drives the link through the compiler,
# so bare ld options need the -Wl, prefix
set(KPHP_CONFDATA_LINK_FLAGS ${RUNTIME_LIGHT_LINK_FLAGS})
if(NOT APPLE)
  list(TRANSFORM KPHP_CONFDATA_LINK_FLAGS REPLACE "^--" "-Wl,--")
endif()
target_link_options(kphp-confdata-pic PUBLIC ${KPHP_CONFDATA_LINK_FLAGS})

string(TIMESTAMP KPHP_CONFDATA_BUILD_TIMESTAMP "%s" UTC)
target_compile_definitions(kphp-confdata-pic PRIVATE KPHP_CONFDATA_BUILD_TIMESTAMP=${KPHP_CONFDATA_BUILD_TIMESTAMP}ULL
                                                   KPHP_CONFDATA_COMPILER_VERSION="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
