if(KPHP_TESTS)
    function(vk_add_unittest TEST_NAME SRC_LIBS)
        set(TEST_NAME unittests-${TEST_NAME})
        add_executable(${TEST_NAME} ${ARGN})
        target_link_libraries(${TEST_NAME} PRIVATE GTest::GTest GTest::Main gmock ${SRC_LIBS} vk::popular_common)
        target_link_options(${TEST_NAME} PRIVATE ${NO_PIE})

        # because of https://github.com/VKCOM/kphp/actions/runs/5463884925/jobs/9945150190
        gtest_discover_tests(${TEST_NAME} PROPERTIES DISCOVERY_TIMEOUT 600)
        set_target_properties(${TEST_NAME} PROPERTIES FOLDER tests)
    endfunction()

    if(APPLE)
        add_link_options(-undefined dynamic_lookup)
    endif()

    enable_testing()
    include(common/common-tests.cmake)
    include(net/net-tests.cmake)
    include(tests/cpp/compiler/compiler-tests.cmake)
    include(tests/cpp/runtime/runtime-tests.cmake)
    include(tests/cpp/server/server-tests.cmake)
endif()
