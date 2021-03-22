prepend(SERVER_TESTS_SOURCES ${BASE_DIR}/tests/cpp/server/
        cluster-name-test.cpp
        confdata-binlog-events-test.cpp
        php-engine-test.cpp)

if(COMPILER_GCC)
    set_source_files_properties(${BASE_DIR}/tests/cpp/server/confdata-binlog-events-test.cpp PROPERTIES COMPILE_FLAGS -Wno-stringop-overflow)
endif()

vk_add_unittest(server "${RUNTIME_LIBS};${RUNTIME_LINK_TEST_LIBS}" ${SERVER_TESTS_SOURCES} ${BASE_DIR}/tests/cpp/runtime/_runtime-tests-env.cpp)
