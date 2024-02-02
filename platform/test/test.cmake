prepend(PLATFORM_TEST_SRC ${BASE_DIR}/platform/test/
        test.cpp)

add_executable(test ${PLATFORM_TEST_SRC})

find_package(GTest REQUIRED)

set_target_properties(test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/objs)

