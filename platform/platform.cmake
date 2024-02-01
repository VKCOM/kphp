prepend(PLATFORM_SRC ${BASE_DIR}/platform/
        main.cpp
        allocator.cpp)

add_executable(platform ${PLATFORM_SRC})
set_target_properties(platform PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/objs)