prepend(KPHP_RUNTIME_MEMORY_RESOURCE_SOURCES memory_resource/
        dealer.cpp
        details/memory_chunk_tree.cpp
        details/memory_ordered_chunk_list.cpp
        heap_resource.cpp
        memory_resource.cpp
        monotonic_buffer_resource.cpp
        unsynchronized_pool_resource.cpp)

prepend(KPHP_RUNTIME_SOURCES ${BASE_DIR}/runtime/
        ${KPHP_RUNTIME_MEMORY_RESOURCE_SOURCES}
        allocator.cpp
        array_functions.cpp
        bcmath.cpp
        confdata-functions.cpp
        confdata-global-manager.cpp
        confdata-keys.cpp
        critical_section.cpp
        curl.cpp
        datetime.cpp
        exception.cpp
        files.cpp
        instance_cache.cpp
        inter-process-mutex.cpp
        interface.cpp
        kphp-backtrace.cpp
        mail.cpp
        math_functions.cpp
        mbstring.cpp
        memcache.cpp
        memory_usage.cpp
        misc.cpp
        msgpack-serialization.cpp
        mysql.cpp
        net_events.cpp
        on_kphp_warning_callback.cpp
        openssl.cpp
        php_assert.cpp
        profiler.cpp
        regexp.cpp
        resumable.cpp
        rpc.cpp
        storage.cpp
        streams.cpp
        string_buffer.cpp
        string_cache.cpp
        string_functions.cpp
        tl/rpc_tl_query.cpp
        tl/rpc_response.cpp
        tl/rpc_server.cpp
        typed_rpc.cpp
        uber-h3.cpp
        udp.cpp
        url.cpp
        vkext.cpp
        vkext_stats.cpp
        zlib.cpp)

set_source_files_properties(
        ${BASE_DIR}/server/php-runner.cpp
        ${BASE_DIR}/server/php-engine.cpp
        ${BASE_DIR}/runtime/interface.cpp
        ${COMMON_DIR}/dl-utils-lite.cpp
        ${COMMON_DIR}/netconf.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-unused-result"
)

set(KPHP_RUNTIME_ALL_SOURCES
    ${KPHP_RUNTIME_SOURCES}
    ${KPHP_SERVER_SOURCES})

set_source_files_properties(${BASE_DIR}/runtime/allocator.cpp PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
set_source_files_properties(${BASE_DIR}/runtime/openssl.cpp PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)

vk_add_library(kphp_runtime OBJECT ${KPHP_RUNTIME_ALL_SOURCES})
target_include_directories(kphp_runtime PUBLIC ${BASE_DIR} /opt/curl7600/include)

set(RUNTIME_LIBS
        vk::kphp_runtime vk::kphp_server vk::popular_common vk::unicode vk::common_src vk::binlog_src vk::net_src
        -l:libyaml-cpp.a -l:libre2.a -l:libzstd.a -l:libh3.a m rt crypto z pthread)
vk_add_library(kphp-full-runtime STATIC)
target_link_libraries(kphp-full-runtime PUBLIC ${RUNTIME_LIBS})
set_target_properties(kphp-full-runtime PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OBJS_DIR})

set(RUNTIME_LINK_TEST_LIBS vk::flex_data_static pcre ssl /opt/curl7600/lib/libcurl.a -l:libnghttp2.a)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp
[[
#include "php_functions.h"
#include "server/php-script.h"
]])

add_library(php_lib_version_j OBJECT ${CMAKE_CURRENT_BINARY_DIR}/php_lib_version.cpp)
target_compile_options(php_lib_version_j PRIVATE -I. -E)
add_dependencies(php_lib_version_j kphp-full-runtime)

add_custom_command(OUTPUT ${OBJS_DIR}/php_lib_version.sha256
        COMMAND tail -n +3 $<TARGET_OBJECTS:php_lib_version_j> | sha256sum | awk '{print $$1}' > ${OBJS_DIR}/php_lib_version.sha256
        DEPENDS php_lib_version_j $<TARGET_OBJECTS:php_lib_version_j>
        COMMENT "php_lib_version.sha256 generation")

