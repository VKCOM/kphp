prepend(COMPILER_TESTS_SOURCES ${BASE_DIR}/tests/cpp/compiler/
        _compiler-tests-env.cpp
        data/performance-inspections-test.cpp
        phpdoc-test.cpp
        lexer-test.cpp)

vk_add_unittest(compiler "${COMPILER_LIBS}" ${COMPILER_TESTS_SOURCES})
