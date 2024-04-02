include(${BASE_DIR}/script/echo/script.cmake)
include(${BASE_DIR}/script/respectful_busy_loop/script.cmake)
include(${BASE_DIR}/script/forward/script.cmake)
include(${BASE_DIR}/script/dev_null/script.cmake)
include(${BASE_DIR}/script/dev_random/script.cmake)

add_custom_target(all_components
        COMMENT "Build all test component"
)

add_dependencies(all_components
        respectful-busy-loop-component
        echo-component
        forward-component
        dev-null-component
        dev-random-component)
