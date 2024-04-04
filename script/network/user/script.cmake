prepend(USER_SRC ${BASE_DIR}/script/network/user/
        user.cpp
        image.cpp)

add_library(network-user-component SHARED ${USER_SRC})
target_include_directories(network-user-component PRIVATE ${BIN_DIR}/runtime)
target_link_libraries(network-user-component PRIVATE vk::popular_common vk::common vk::runtimelight)
set_target_properties(network-user-component PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${BASE_DIR}/objs
        POSITION_INDEPENDENT_CODE ON)