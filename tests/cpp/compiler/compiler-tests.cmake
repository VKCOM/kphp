prepend(COMPILER_TESTS_SOURCES ${BASE_DIR}/tests/cpp/compiler/
        _compiler-tests-env.cpp
        data/performance-inspections-test.cpp
        phpdoc-test.cpp
        typedata-test.cpp
        lexer-test.cpp
        ffi-parser-test.cpp
        utils/string-utils-test.cpp)

vk_add_unittest(compiler "${COMPILER_LIBS}" ${COMPILER_TESTS_SOURCES})
