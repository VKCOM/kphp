prepend(DEV_NULL_SRC ${BASE_DIR}/script/dev_null/
        dev_null.cpp
        image.cpp)

add_library(dev-null-component SHARED ${DEV_NULL_SRC})
target_include_directories(dev-null-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(dev-null-component PRIVATE vk::popular_common vk::runtimelight)
set_target_properties(dev-null-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)