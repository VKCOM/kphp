set(RUNTIME_LIGHT_CONFDATA_TEST_SOURCES
    ${BASE_DIR}/runtime-light/components/confdata/state/predefined-wildcards-builder.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/confdata-keys.cpp
    ${BASE_DIR}/runtime-light/stdlib/confdata/predefined-wildcards.cpp
    ${BASE_DIR}/tests/cpp/runtime-light/confdata/predefined-wildcards-test.cpp)

add_executable(unittests-runtime-light-confdata ${RUNTIME_LIGHT_CONFDATA_TEST_SOURCES})
target_compile_options(unittests-runtime-light-confdata PRIVATE ${RUNTIME_LIGHT_COMPILE_FLAGS})
target_link_options(unittests-runtime-light-confdata PRIVATE -stdlib=libc++)
add_test(NAME unittests-runtime-light-confdata COMMAND unittests-runtime-light-confdata)
set_target_properties(unittests-runtime-light-confdata PROPERTIES FOLDER tests)
