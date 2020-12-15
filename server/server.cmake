prepend(KPHP_SERVER_SOURCES ${BASE_DIR}/server/
        confdata-binlog-replay.cpp
        confdata-stats.cpp
        json-logger.cpp
        lease-config-parser.cpp
        lease-rpc-client.cpp
        php-engine-vars.cpp
        php-engine.cpp
        php-lease.cpp
        php-master.cpp
        php-master-tl-handlers.cpp
        php-mc-connections.cpp
        php-queries.cpp
        php-query-data.cpp
        php-runner.cpp
        php-script.cpp
        php-sql-connections.cpp
        php-worker-stats.cpp)

allow_deprecated_declarations_for_apple(${BASE_DIR}/server/php-runner.cpp)
vk_add_library(kphp_server OBJECT ${KPHP_SERVER_SOURCES})
