include(${BASE_DIR}/script/network/user/script.cmake)
include(${BASE_DIR}/script/network/server/script.cmake)
include(${BASE_DIR}/script/network/db/script.cmake)

add_custom_target(network-components)

add_dependencies(network-components
        network-user-component
        network-server-component
        network-db-component)
