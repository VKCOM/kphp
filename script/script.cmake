prepend(SCRIPT_SRC ${BASE_DIR}/script/
        src_dev2ecd55ec1895a080.cpp
        demo.cpp)

add_library(script_src OBJECT ${SCRIPT_SRC})

target_include_directories(script_src PRIVATE ${BIN_DIR}/runtime)
set_property(TARGET script_src PROPERTY POSITION_INDEPENDENT_CODE ON)