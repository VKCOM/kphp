prepend(RESPECTFUL_BUSY_LOOP_SRC ${BASE_DIR}/script/respectful_busy_loop/
        respectful_busy_loop.cpp
        image.cpp)

add_library(respectful_busy_loop_src STATIC ${RESPECTFUL_BUSY_LOOP_SRC})
target_include_directories(respectful_busy_loop_src PRIVATE ${BIN_DIR}/runtime)
set_property(TARGET respectful_busy_loop_src PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(respectful-busy-loop-src-component SHARED $<TARGET_OBJECTS:runtimelight> $<TARGET_OBJECTS:common_src> $<TARGET_OBJECTS:respectful_busy_loop_src>)
set_target_properties(respectful-busy-loop-src-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs)
