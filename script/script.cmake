include(${BASE_DIR}/script/echo/script.cmake)
include(${BASE_DIR}/script/respectful_busy_loop/script.cmake)

add_custom_target(all_components
        COMMENT "Build all test component"
)

add_dependencies(all_components respectful-busy-loop-src-component echo-component)
