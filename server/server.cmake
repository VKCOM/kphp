prepend(KPHP_SERVER_SOURCES ${BASE_DIR}/server/
        master-name.cpp
        confdata-binlog-replay.cpp
        confdata-stats.cpp
        curl-adaptor.cpp
        shared-data.cpp
        http-server-context.cpp
        json-logger.cpp
        lease-config-parser.cpp
        lease-rpc-client.cpp
        numa-configuration.cpp
        php-engine-vars.cpp
        php-engine.cpp
        php-lease.cpp
        php-master.cpp
        php-master-restart.cpp
        php-master-tl-handlers.cpp
        php-mc-connections.cpp
        php-queries.cpp
        php-queries-types.cpp
        php-query-data.cpp
        php-runner.cpp
        php-init-scripts.cpp
        php-sql-connections.cpp
        php-worker.cpp
        server-config.cpp
        server-log.cpp
        server-stats.cpp
        slot-ids-factory.cpp
        workers-control.cpp
        shared-data-worker-cache.cpp
        signal-handlers.cpp
        statshouse/statshouse-client.cpp
        statshouse/add-metrics-batch.cpp
        statshouse/worker-stats-buffer.cpp)

prepend(KPHP_JOB_WORKERS_SOURCES ${BASE_DIR}/server/job-workers/
        job-stats.cpp
        job-worker-server.cpp
        job-worker-client.cpp
        job-workers-context.cpp
        pipe-io.cpp
        shared-memory-manager.cpp)

prepend(KPHP_DATABASE_DRIVERS_SOURCES ${BASE_DIR}/server/database-drivers/
        adaptor.cpp
        connector.cpp)

if (PDO_DRIVER_MYSQL)
prepend(KPHP_DATABASE_DRIVERS_MYSQL_SOURCES ${BASE_DIR}/server/database-drivers/mysql/
        mysql.cpp
        mysql-request.cpp
        mysql-connector.cpp
        mysql-response.cpp
        mysql-resources.cpp)
endif()

if (PDO_DRIVER_PGSQL)
prepend(KPHP_DATABASE_DRIVERS_PGSQL_SOURCES ${BASE_DIR}/server/database-drivers/pgsql/
        pgsql.cpp
        pgsql-request.cpp
        pgsql-connector.cpp
        pgsql-response.cpp
        pgsql-resources.cpp)
endif()

set(KPHP_SERVER_ALL_SOURCES
    ${KPHP_SERVER_SOURCES}
    ${KPHP_JOB_WORKERS_SOURCES}
    ${KPHP_DATABASE_DRIVERS_SOURCES}
    ${KPHP_DATABASE_DRIVERS_MYSQL_SOURCES}
    ${KPHP_DATABASE_DRIVERS_PGSQL_SOURCES})

allow_deprecated_declarations_for_apple(${BASE_DIR}/server/php-runner.cpp)
vk_add_library(kphp_server OBJECT ${KPHP_SERVER_ALL_SOURCES})
