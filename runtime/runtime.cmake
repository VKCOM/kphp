# Only runtime-related third-parties
if(NOT APPLE)
    include(${THIRD_PARTY_DIR}/numactl-cmake/numactl.cmake)
    set(NUMA_LIB NUMACTL::${PIC_MODE}::numactl)
    set(NUMA_LIB_PIC NUMACTL::pic::numactl)
    set(NUMA_LIB_NO_PIC NUMACTL::no-pic::numactl)
endif()
include(${THIRD_PARTY_DIR}/timelib-cmake/timelib.cmake)
include(${THIRD_PARTY_DIR}/uber-h3-cmake/uber-h3.cmake)
include(${THIRD_PARTY_DIR}/pcre-cmake/pcre.cmake)
include(${THIRD_PARTY_DIR}/nghttp2-cmake/nghttp2.cmake)
include(${THIRD_PARTY_DIR}/curl-cmake/curl.cmake)

prepend(KPHP_RUNTIME_DATETIME_SOURCES datetime/
        date_interval.cpp
        datetime.cpp
        datetime_functions.cpp
        datetime_immutable.cpp
        datetime_zone.cpp
        timelib_wrapper.cpp)

prepend(KPHP_RUNTIME_MEMORY_IMPL_RESOURCE_SOURCES memory_resource_impl/
        dealer.cpp
        heap_resource.cpp
        memory_resource_stats.cpp
        monotonic_runtime_buffer_resource.cpp)

prepend(KPHP_RUNTIME_JOB_WORKERS_SOURCES job-workers/
        client-functions.cpp
        job-interface.cpp
        processing-jobs.cpp
        server-functions.cpp)

prepend(KPHP_RUNTIME_ML_SOURCES kphp_ml/
        kphp_ml.cpp
        kphp_ml_catboost.cpp
        kphp_ml_xgboost.cpp
        kphp_ml_init.cpp
        kphp_ml_interface.cpp
        kml-files-reader.cpp)

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
        ${KPHP_RUNTIME_MEMORY_IMPL_RESOURCE_SOURCES}
        ${KPHP_RUNTIME_JOB_WORKERS_SOURCES}
        ${KPHP_RUNTIME_SPL_SOURCES}
        ${KPHP_RUNTIME_ML_SOURCES}
        ${KPHP_RUNTIME_PDO_SOURCES}
        ${KPHP_RUNTIME_PDO_MYSQL_SOURCES}
        ${KPHP_RUNTIME_PDO_PGSQL_SOURCES}
        allocator.cpp
        context/runtime-core-allocator.cpp
        context/runtime-context.cpp
        array_functions.cpp
        bcmath.cpp
        math-context.cpp
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
        instance-cache.cpp
        instance-copy-processor.cpp
        inter-process-mutex.cpp
        interface.cpp
        kphp-backtrace.cpp
        kphp_tracing.cpp
        kphp_tracing_binlog.cpp
        mail.cpp
        math_functions.cpp
        memcache.cpp
        memory_usage.cpp
        misc.cpp
        mysql.cpp
        net_events.cpp
        on_kphp_warning_callback.cpp
        oom_handler.cpp
        openssl.cpp
        php_assert.cpp
        php-script-globals.cpp
        profiler.cpp
        regexp.cpp
        resumable.cpp
        rpc.cpp
        rpc_extra_info.cpp
        serialize-context.cpp
        storage.cpp
        streams.cpp
        string_functions.cpp
        string-context.cpp
        tl/rpc_req_error.cpp
        tl/rpc_tl_query.cpp
        tl/rpc_response.cpp
        tl/rpc_server.cpp
        tl/tl_magics_decoding.cpp
        typed_rpc.cpp
        uber-h3.cpp
        udp.cpp
        tcp.cpp
        vkext.cpp
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

# Suppress YAML-cpp-related warnings
if(COMPILER_CLANG)
    allow_deprecated_declarations(${BASE_DIR}/runtime/interface.cpp)
endif()

set(KPHP_RUNTIME_ALL_SOURCES
    ${KPHP_RUNTIME_SOURCES}
    ${KPHP_SERVER_SOURCES})

allow_deprecated_declarations(${BASE_DIR}/runtime/allocator.cpp ${BASE_DIR}/runtime/openssl.cpp)
allow_deprecated_declarations_for_apple(${BASE_DIR}/runtime/inter-process-mutex.cpp)

#### NO PIC
vk_add_library_no_pic(kphp-runtime-no-pic STATIC ${KPHP_RUNTIME_ALL_SOURCES})
target_include_directories(kphp-runtime-no-pic PUBLIC ${BASE_DIR})

set(RUNTIME_LIBS_NO_PIC
        vk::no-pic::kphp-server
        vk::no-pic::runtime-common
        vk::no-pic::popular-common
        vk::no-pic::unicode
        vk::no-pic::common-src
        vk::no-pic::binlog-src
        vk::no-pic::net-src
        CURL::no-pic::curl
        OpenSSL::no-pic::SSL
        OpenSSL::no-pic::Crypto
        ZLIB::no-pic::zlib
        NGHTTP2::no-pic::nghttp2
        ZSTD::no-pic::zstd
        RE2::no-pic::re2
        PCRE::no-pic::pcre
        UBER_H3::no-pic::uber-h3
        KPHP_TIMELIB::no-pic::timelib
        YAML_CPP::no-pic::yaml-cpp
        ${NUMA_LIB_NO_PIC}
        m
        pthread
)
target_link_libraries(kphp-runtime-no-pic PUBLIC ${RUNTIME_LIBS_NO_PIC})

add_dependencies(kphp-runtime-no-pic KPHP_TIMELIB::no-pic::timelib OpenSSL::no-pic::Crypto OpenSSL::no-pic::SSL CURL::no-pic::curl NGHTTP2::no-pic::nghttp2 ZLIB::no-pic::zlib ZSTD::no-pic::zstd RE2::no-pic::re2 PCRE::no-pic::pcre UBER_H3::no-pic::uber-h3 YAML_CPP::no-pic::yaml-cpp ${NUMA_LIB_NO_PIC})
combine_static_runtime_library(kphp-runtime-no-pic kphp-full-runtime-no-pic)
###

#### PIC
vk_add_library_pic(kphp-runtime-pic STATIC ${KPHP_RUNTIME_ALL_SOURCES})
target_include_directories(kphp-runtime-pic PUBLIC ${BASE_DIR})

set(RUNTIME_LIBS_PIC
        vk::pic::kphp-server
        vk::pic::runtime-common
        vk::pic::popular-common
        vk::pic::unicode
        vk::pic::common-src
        vk::pic::binlog-src
        vk::pic::net-src
        CURL::pic::curl
        OpenSSL::pic::SSL
        OpenSSL::pic::Crypto
        ZLIB::pic::zlib
        NGHTTP2::pic::nghttp2
        ZSTD::pic::zstd
        RE2::pic::re2
        PCRE::pic::pcre
        UBER_H3::pic::uber-h3
        KPHP_TIMELIB::pic::timelib
        YAML_CPP::pic::yaml-cpp
        ${NUMA_LIB_PIC}
        m
        pthread
)
target_link_libraries(kphp-runtime-pic PUBLIC ${RUNTIME_LIBS_PIC})

add_dependencies(kphp-runtime-pic KPHP_TIMELIB::pic::timelib OpenSSL::pic::Crypto OpenSSL::pic::SSL CURL::pic::curl NGHTTP2::pic::nghttp2 ZLIB::pic::zlib ZSTD::pic::zstd RE2::pic::re2 PCRE::pic::pcre UBER_H3::pic::uber-h3 YAML_CPP::pic::yaml-cpp ${NUMA_LIB_PIC})
combine_static_runtime_library(kphp-runtime-pic kphp-full-runtime-pic)
###

set(RUNTIME_LIBS
        vk::${PIC_MODE}::kphp-runtime
        vk::${PIC_MODE}::kphp-server
        vk::${PIC_MODE}::runtime-common
        vk::${PIC_MODE}::popular-common
        vk::${PIC_MODE}::unicode
        vk::${PIC_MODE}::common-src
        vk::${PIC_MODE}::binlog-src
        vk::${PIC_MODE}::net-src
        CURL::${PIC_MODE}::curl
        OpenSSL::${PIC_MODE}::SSL
        OpenSSL::${PIC_MODE}::Crypto
        ZLIB::${PIC_MODE}::zlib
        NGHTTP2::${PIC_MODE}::nghttp2
        ZSTD::${PIC_MODE}::zstd
        RE2::${PIC_MODE}::re2
        YAML_CPP::${PIC_MODE}::yaml-cpp
        m
        pthread
)

set(RUNTIME_LINK_TEST_LIBS
        vk::${PIC_MODE}::flex-data-src
        CURL::${PIC_MODE}::curl
        OpenSSL::${PIC_MODE}::SSL
        NGHTTP2::${PIC_MODE}::nghttp2
        PCRE::${PIC_MODE}::pcre
        ${NUMA_LIB}
        KPHP_TIMELIB::${PIC_MODE}::timelib
        ${EPOLL_SHIM_LIB}
        ${ICONV_LIB}
        ${RT_LIB}
        dl
)

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
file(GLOB_RECURSE KPHP_RUNTIME_COMMON_ALL_HEADERS
     RELATIVE ${BASE_DIR}
     CONFIGURE_DEPENDS
     "${BASE_DIR}/runtime-common/*.h")
list(APPEND KPHP_RUNTIME_ALL_HEADERS ${KPHP_RUNTIME_COMMON_ALL_HEADERS})
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
target_include_directories(php_lib_version_j PUBLIC ${ZLIB_NO_PIC_INCLUDE_DIRS} ${PCRE_NO_PIC_INCLUDE_DIRS})
target_compile_options(php_lib_version_j PRIVATE -I. -E)
add_dependencies(php_lib_version_j kphp-full-runtime-no-pic kphp-full-runtime-pic)

add_custom_command(OUTPUT ${OBJS_DIR}/php_lib_version.sha256
        COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
        DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
        COMMENT "php_lib_version.sha256 generation")

# this is to create a dummy file so the compiler can find it and include
configure_file(${BASE_DIR}/compiler/runtime_link_libs.h.in
               ${AUTO_DIR}/compiler/runtime_link_libs.h)
