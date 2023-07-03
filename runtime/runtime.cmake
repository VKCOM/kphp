prepend(KPHP_RUNTIME_DATETIME_SOURCES datetime/
        date_interval.cpp
        datetime.cpp
        datetime_functions.cpp
        datetime_immutable.cpp
        datetime_zone.cpp
        timelib_wrapper.cpp)

prepend(KPHP_RUNTIME_MEMORY_RESOURCE_SOURCES memory_resource/
        dealer.cpp
        details/memory_chunk_tree.cpp
        details/memory_ordered_chunk_list.cpp
        heap_resource.cpp
        memory_resource.cpp
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp)

prepend(KPHP_RUNTIME_MSGPACK_SOURCES msgpack/
        check_instance_depth.cpp
        object_visitor.cpp
        packer.cpp
        parser.cpp
        unpacker.cpp
        zone.cpp)

prepend(KPHP_RUNTIME_JOB_WORKERS_SOURCES job-workers/
        client-functions.cpp
        job-interface.cpp
        processing-jobs.cpp
        server-functions.cpp)

prepend(KPHP_RUNTIME_SPL_SOURCES spl/
        array_iterator.cpp)

prepend(KPHP_RUNTIME_PDO_SOURCES pdo/
        pdo.cpp
        pdo_statement.cpp
        abstract_pdo_driver.cpp)

if(PDO_DRIVER_MYSQL)
prepend(KPHP_RUNTIME_PDO_MYSQL_SOURCES pdo/mysql/
        mysql_pdo_driver.cpp
        mysql_pdo_emulated_statement.cpp)
endif()

if (PDO_DRIVER_PGSQL)
prepend(KPHP_RUNTIME_PDO_PGSQL_SOURCES pdo/pgsql/
        pgsql_pdo_driver.cpp
        pgsql_pdo_emulated_statement.cpp)
endif()

prepend(KPHP_RUNTIME_SOURCES ${BASE_DIR}/runtime/
        ${KPHP_RUNTIME_DATETIME_SOURCES}
        ${KPHP_RUNTIME_MEMORY_RESOURCE_SOURCES}
        ${KPHP_RUNTIME_MSGPACK_SOURCES}
        ${KPHP_RUNTIME_JOB_WORKERS_SOURCES}
        ${KPHP_RUNTIME_SPL_SOURCES}
        ${KPHP_RUNTIME_PDO_SOURCES}
        ${KPHP_RUNTIME_PDO_MYSQL_SOURCES}
        ${KPHP_RUNTIME_PDO_PGSQL_SOURCES}
        allocator.cpp
        array_functions.cpp
        bcmath.cpp
        common_template_instantiations.cpp
        confdata-functions.cpp
        confdata-global-manager.cpp
        confdata-keys.cpp
        critical_section.cpp
        ctype.cpp
        curl.cpp
        curl-async.cpp
        env.cpp
        exception.cpp
        exec.cpp
        files.cpp
        from-json-processor.cpp
        instance-cache.cpp
        instance-copy-processor.cpp
        inter-process-mutex.cpp
        interface.cpp
        json-functions.cpp
        json-writer.cpp
        kphp-backtrace.cpp
        kphp_tracing.cpp
        kphp_tracing_binlog.cpp
        mail.cpp
        math_functions.cpp
        mbstring.cpp
        memcache.cpp
        memory_usage.cpp
        migration_php8.cpp
        misc.cpp
        mixed.cpp
        mysql.cpp
        net_events.cpp
        on_kphp_warning_callback.cpp
        oom_handler.cpp
        openssl.cpp
        php_assert.cpp
        profiler.cpp
        regexp.cpp
        resumable.cpp
        rpc.cpp
        serialize-functions.cpp
        storage.cpp
        streams.cpp
        string.cpp
        string_buffer.cpp
        string_cache.cpp
        string_functions.cpp
        tl/rpc_tl_query.cpp
        tl/rpc_response.cpp
        tl/rpc_server.cpp
        tl/tl_magics_decoding.cpp
        typed_rpc.cpp
        uber-h3.cpp
        udp.cpp
        tcp.cpp
        url.cpp
        vkext.cpp
        vkext_stats.cpp
        ffi.cpp
        zlib.cpp
        zstd.cpp)

set_source_files_properties(
        ${BASE_DIR}/server/php-engine.cpp
        ${BASE_DIR}/server/signal-handlers.cpp
        ${BASE_DIR}/server/json-logger.cpp
        ${BASE_DIR}/runtime/interface.cpp
        ${COMMON_DIR}/dl-utils-lite.cpp
        ${COMMON_DIR}/netconf.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-unused-result"
)

set(KPHP_RUNTIME_ALL_SOURCES
    ${KPHP_RUNTIME_SOURCES}
    ${KPHP_SERVER_SOURCES})

allow_deprecated_declarations(${BASE_DIR}/runtime/allocator.cpp ${BASE_DIR}/runtime/openssl.cpp)
allow_deprecated_declarations_for_apple(${BASE_DIR}/runtime/inter-process-mutex.cpp)

vk_add_library(kphp_runtime OBJECT ${KPHP_RUNTIME_ALL_SOURCES})
target_include_directories(kphp_runtime PUBLIC ${BASE_DIR} /opt/curl7600/include)

add_dependencies(kphp_runtime kphp-timelib)

prepare_cross_platform_libs(RUNTIME_LIBS yaml-cpp re2 zstd h3) # todo: linking between static libs is no-op, is this redundant? do we need to add mysqlclient here?
set(RUNTIME_LIBS vk::kphp_runtime vk::kphp_server vk::popular_common vk::unicode vk::common_src vk::binlog_src vk::net_src ${RUNTIME_LIBS} OpenSSL::Crypto m z pthread)
vk_add_library(kphp-full-runtime STATIC)
target_link_libraries(kphp-full-runtime PUBLIC ${RUNTIME_LIBS})
set_target_properties(kphp-full-runtime PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OBJS_DIR})

prepare_cross_platform_libs(RUNTIME_LINK_TEST_LIBS pcre nghttp2 kphp-timelib)
set(RUNTIME_LINK_TEST_LIBS vk::flex_data_static ${CURL_LIB} OpenSSL::SSL ${NUMA_LIB} ${RUNTIME_LINK_TEST_LIBS} ${EPOLL_SHIM_LIB} ${ICONV_LIB} ${RT_LIB})

if (PDO_DRIVER_MYSQL)
    list(APPEND RUNTIME_LINK_TEST_LIBS mysqlclient)
endif()

if (PDO_DRIVER_PGSQL)
    list(APPEND RUNTIME_LINK_TEST_LIBS PostgreSQL::PostgreSQL)
endif()

file(GLOB_RECURSE KPHP_RUNTIME_ALL_HEADERS
     RELATIVE ${BASE_DIR}
     CONFIGURE_DEPENDS
     "${BASE_DIR}/runtime/*.h")
list(TRANSFORM KPHP_RUNTIME_ALL_HEADERS REPLACE "^(.+)$" [[#include "\1"]])
list(JOIN KPHP_RUNTIME_ALL_HEADERS "\n" MERGED_RUNTIME_HEADERS)
file(WRITE ${AUTO_DIR}/runtime/runtime-headers.h "\
#ifndef MERGED_RUNTIME_HEADERS_H
#define MERGED_RUNTIME_HEADERS_H

${MERGED_RUNTIME_HEADERS}

#endif
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp
[[
#include "auto/runtime/runtime-headers.h"
#include "server/php-init-scripts.h"
]])

add_library(php_lib_version_j OBJECT ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp)
target_compile_options(php_lib_version_j PRIVATE -I. -E)
add_dependencies(php_lib_version_j kphp-full-runtime)

add_custom_command(OUTPUT ${OBJS_DIR}/php_lib_version.sha256
        COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
        DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
        COMMENT "php_lib_version.sha256 generation")

