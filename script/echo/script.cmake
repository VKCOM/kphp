prepend(ECHO_SRC ${BASE_DIR}/script/echo/
        echo.cpp
        image.cpp)

add_library(echo-component SHARED ${ECHO_SRC})
target_include_directories(echo-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(echo-component PRIVATE vk::light_common vk::runtime_light)
set_target_properties(echo-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)