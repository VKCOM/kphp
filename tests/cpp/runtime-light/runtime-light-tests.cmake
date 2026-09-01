set(RUNTIME_LIGHT_CONFDATA_TEST_RUNTIME_CORE_SOURCES ${CORE_SRC})
list(TRANSFORM RUNTIME_LIGHT_CONFDATA_TEST_RUNTIME_CORE_SOURCES PREPEND "${RUNTIME_COMMON_DIR}/")

set(RUNTIME_LIGHT_CONFDATA_TEST_SOURCES
    ${RUNTIME_LIGHT_CONFDATA_TEST_RUNTIME_CORE_SOURCES}
    ${BASE_DIR}/runtime-light/allocator/runtime-light-allocator.cpp
    ${BASE_DIR}/runtime-light/memory-resource-impl/monotonic-light-buffer-resource.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/confdata-storage.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/confdata-keys.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/predefined-wildcards.cpp
    ${BASE_DIR}/tests/cpp/runtime-light/confdata/predefined-wildcards-test.cpp)

add_executable(unittests-runtime-light-confdata ${RUNTIME_LIGHT_CONFDATA_TEST_SOURCES})
target_compile_options(unittests-runtime-light-confdata PRIVATE ${RUNTIME_LIGHT_COMPILE_FLAGS})
target_link_options(unittests-runtime-light-confdata PRIVATE -stdlib=libc++)
add_test(NAME unittests-runtime-light-confdata COMMAND unittests-runtime-light-confdata)
set_target_properties(unittests-runtime-light-confdata PROPERTIES FOLDER tests)

set(RUNTIME_LIGHT_ALLOCATOR_TEST_RUNTIME_CORE_SOURCES ${CORE_SRC})
list(TRANSFORM RUNTIME_LIGHT_ALLOCATOR_TEST_RUNTIME_CORE_SOURCES PREPEND "${RUNTIME_COMMON_DIR}/")

set(RUNTIME_LIGHT_ALLOCATOR_TEST_SOURCES
    ${RUNTIME_LIGHT_ALLOCATOR_TEST_RUNTIME_CORE_SOURCES}
    ${BASE_DIR}/runtime-common/stdlib/serialization/json-functions.cpp
    ${BASE_DIR}/runtime-common/stdlib/serialization/serialize-functions.cpp
    ${BASE_DIR}/runtime-light/allocator/runtime-light-allocator.cpp
    ${BASE_DIR}/runtime-light/memory-resource-impl/monotonic-light-buffer-resource.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/confdata-storage.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/confdata-keys.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/predefined-wildcards.cpp
    ${BASE_DIR}/tests/cpp/runtime-light/allocator/script-memory-resource-test.cpp)

add_executable(unittests-runtime-light-allocator ${RUNTIME_LIGHT_ALLOCATOR_TEST_SOURCES})
target_compile_options(unittests-runtime-light-allocator PRIVATE ${RUNTIME_LIGHT_COMPILE_FLAGS})
target_link_options(unittests-runtime-light-allocator PRIVATE -stdlib=libc++)
add_test(NAME unittests-runtime-light-allocator COMMAND unittests-runtime-light-allocator)
set_target_properties(unittests-runtime-light-allocator PROPERTIES FOLDER tests)
