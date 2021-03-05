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
        php-master-restart.cpp
        php-master-tl-handlers.cpp
        php-mc-connections.cpp
        php-queries.cpp
        php-query-data.cpp
        php-runner.cpp
        php-script.cpp
        php-sql-connections.cpp
        php-worker-stats.cpp
        slot-ids-factory.cpp)

prepend(KPHP_JOB_WORKERS_SOURCES ${BASE_DIR}/server/job-workers/
        job-worker-server.cpp
        job-worker-client.cpp
        job-workers-context.cpp
        pipe-io.cpp
        shared-context.cpp
        simple-php-script.cpp)

set(KPHP_SERVER_ALL_SOURCES
    ${KPHP_SERVER_SOURCES}
    ${KPHP_JOB_WORKERS_SOURCES})

allow_deprecated_declarations_for_apple(${BASE_DIR}/server/php-runner.cpp)
vk_add_library(kphp_server OBJECT ${KPHP_SERVER_ALL_SOURCES})
