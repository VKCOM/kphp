# Standalone bindings are linked as objects so they are available before the
# runtime archive is searched, without pulling in production component bindings.
add_library(runtime-light-test-support OBJECT
    ${BASE_DIR}/tests/cpp/runtime-light/runtime-context.cpp
    ${BASE_DIR}/tests/cpp/runtime-light/array-access-stubs.cpp
    ${BASE_DIR}/tests/cpp/runtime-light/diagnostics-stubs.cpp)
target_link_libraries(runtime-light-test-support PUBLIC kphp-light-runtime-pic)
set_target_properties(runtime-light-test-support PROPERTIES FOLDER tests)

function(add_runtime_light_unittest TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE runtime-light-test-support)
    # These executables do not host dynamically loaded components. Do not retain
    # unused runtime entry points merely to export them to potential plugins.
    if(NOT APPLE)
        target_link_options(${TEST_NAME} PRIVATE -Wl,--no-export-dynamic)
    endif()
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_target_properties(${TEST_NAME} PROPERTIES FOLDER tests)
endfunction()

add_runtime_light_unittest(unittests-runtime-light-confdata
    ${BASE_DIR}/tests/cpp/runtime-light/confdata/confdata-test.cpp)
add_runtime_light_unittest(unittests-runtime-light-allocator
    ${BASE_DIR}/tests/cpp/runtime-light/allocator/allocator-test.cpp)

# This file is included only when KPHP_TESTS is enabled. The allocator suite
# also covers confdata storage, sample lifetimes, and clean-sync size hints.
add_dependencies(kphp-confdata-pic
    unittests-runtime-light-confdata
    unittests-runtime-light-allocator)
