prepend(FORWARD_SRC ${BASE_DIR}/script/forward/
        forward.cpp
        image.cpp)

add_library(forward-component SHARED ${FORWARD_SRC})
target_include_directories(forward-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(forward-component PRIVATE vk::popular_common vk::runtimelight)
set_target_properties(forward-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)
