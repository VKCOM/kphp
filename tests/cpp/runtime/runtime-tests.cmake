prepend(RUNTIME_TESTS_SOURCES ${BASE_DIR}/tests/cpp/runtime/
		_runtime-tests-env.cpp
		allocator-malloc-replacement-test.cpp
		array-test.cpp
		common-php-functions-test.cpp
		confdata-functions-test.cpp
		confdata-key-maker-test.cpp
		confdata-predefined-wildcards-test.cpp
		flex-test.cpp
		inter-process-mutex-test.cpp
		inter-process-resource-test.cpp
		json-writer-test.cpp
		number-string-comparison.cpp
		kphp-type-traits-test.cpp
		msgpack-test.cpp
		memory_resource/details/memory_chunk_list-test.cpp
		memory_resource/details/memory_chunk_tree-test.cpp
		memory_resource/details/memory_ordered_chunk_list-test.cpp
		memory_resource/extra-memory-pool-test.cpp
		memory_resource/unsynchronized_pool_resource-test.cpp
		string-list-test.cpp
		string-test.cpp
		zstd-test.cpp
		mbstring-test.cpp)

allow_deprecated_declarations_for_apple(${BASE_DIR}/tests/cpp/runtime/inter-process-mutex-test.cpp)
vk_add_unittest(runtime "${RUNTIME_LIBS};${RUNTIME_LINK_TEST_LIBS}" ${RUNTIME_TESTS_SOURCES})
