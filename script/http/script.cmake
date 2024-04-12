prepend(HTTP_SRC ${BASE_DIR}/script/http/
        http.cpp
        image.cpp)

add_library(http-component SHARED ${HTTP_SRC})
target_include_directories(http-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(http-component PRIVATE vk::popular_common vk::runtimelight)
set_target_properties(http-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)