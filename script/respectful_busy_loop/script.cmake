prepend(RESPECTFUL_BUSY_LOOP_SRC ${BASE_DIR}/script/respectful_busy_loop/
        respectful_busy_loop.cpp
        image.cpp)

add_library(respectful-busy-loop-component SHARED ${RESPECTFUL_BUSY_LOOP_SRC})
target_include_directories(respectful-busy-loop-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(respectful-busy-loop-component PRIVATE vk::light_common vk::runtime_light)
set_target_properties(respectful-busy-loop-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)
