prepend(RESPECTFUL_BUSY_LOOP_SRC ${BASE_DIR}/script/respectful_busy_loop/
        respectful_busy_loop.cpp
        image.cpp)

add_library(respectful-busy-loop-src-component SHARED ${RESPECTFUL_BUSY_LOOP_SRC})
target_include_directories(respectful-busy-loop-src-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(respectful-busy-loop-src-component PRIVATE vk::popular_common vk::common vk::runtimelight)
set_target_properties(respectful-busy-loop-src-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)
