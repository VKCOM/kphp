prepend(DEV_RANDOM_SRC ${BASE_DIR}/script/dev_random/
        dev_random.cpp
        image.cpp)

add_library(dev-random-component SHARED ${DEV_RANDOM_SRC})
target_include_directories(dev-random-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(dev-random-component PRIVATE vk::light_common vk::runtime_light)
target_include_directories(dev-random-component PRIVATE . ${AUTO_DIR}/runtime/)
set_target_properties(dev-random-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)