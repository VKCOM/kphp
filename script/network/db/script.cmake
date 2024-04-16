prepend(DATABASE_SRC ${BASE_DIR}/script/network/db/
        database.cpp
        image.cpp)

add_library(network-db-component SHARED ${DATABASE_SRC})
target_include_directories(network-db-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(network-db-component PRIVATE vk::light_common vk::runtime_light)
target_include_directories(network-db-component PRIVATE . ${AUTO_DIR}/runtime/)
set_target_properties(network-db-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)