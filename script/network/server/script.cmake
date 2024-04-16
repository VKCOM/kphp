prepend(SERVER_SRC ${BASE_DIR}/script/network/server/
        server.cpp
        image.cpp)

add_library(network-server-component SHARED ${SERVER_SRC})
target_include_directories(network-server-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(network-server-component PRIVATE vk::light_common vk::runtime_light)
target_include_directories(network-server-component PRIVATE . ${AUTO_DIR}/runtime/)
set_target_properties(network-server-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)