prepend(UTILS_SRC ${BASE_DIR}/platform/utils/
        allocator.cpp)

if(DEBUG)
    add_definitions(-DDEBUG)
endif()

add_library(utils_src OBJECT ${UTILS_SRC})
set_property(TARGET utils_src PROPERTY POSITION_INDEPENDENT_CODE ON)

include(${BASE_DIR}/platform/test/test.cmake)


add_executable(platform ${BASE_DIR}/platform/main.cpp)
set_target_properties(platform PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/objs)