prepend(ECHO_SRC ${BASE_DIR}/script/echo/
        echo.cpp
        image.cpp)

add_library(echo_src OBJECT ${ECHO_SRC})
target_include_directories(echo_src PRIVATE ${BIN_DIR}/runtime)
set_property(TARGET echo_src PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(echo-component SHARED $<TARGET_OBJECTS:runtimelight> $<TARGET_OBJECTS:common_src> $<TARGET_OBJECTS:echo_src>)
set_target_properties(echo-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs)