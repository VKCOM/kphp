include_guard(GLOBAL)

set(LIGHT_COMMON_SOURCES
        algorithms/simd-int-to-string.cpp
)

set(COMMON_SOURCES_FOR_COMP "${LIGHT_COMMON_SOURCES}")
configure_file(${BASE_DIR}/compiler/common_sources.h.in ${AUTO_DIR}/compiler/common_sources.h)

prepend(LIGHT_COMMON_SOURCES ${COMMON_DIR}/ ${LIGHT_COMMON_SOURCES})

prepend(POPULAR_COMMON_SOURCES ${COMMON_DIR}/
        resolver.cpp
        precise-time.cpp
        cpuid.cpp
        crc32.cpp
        crc32c.cpp
        options.cpp
        secure-bzero.cpp
        crc32_${CMAKE_SYSTEM_PROCESSOR}.cpp
        crc32c_${CMAKE_SYSTEM_PROCESSOR}.cpp
        version-string.cpp
        kernel-version.cpp
        kprintf.cpp
        rpc-headers.cpp
        parallel/counter.cpp
        parallel/maximum.cpp
        parallel/thread-id.cpp
        parallel/limit-counter.cpp
        server/limits.cpp
        server/signals.cpp
        server/relogin.cpp
        server/crash-dump.cpp
        server/engine-settings.cpp
        stats/buffer.cpp
        stats/provider.cpp)

if(APPLE)
    list(APPEND POPULAR_COMMON_SOURCES ${COMMON_DIR}/macos-ports.cpp)
endif()

vk_add_library(light_common OBJECT ${LIGHT_COMMON_SOURCES})
set_property(TARGET light_common PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(light_common PUBLIC -stdlib=libc++)
target_link_options(light_common PUBLIC -stdlib=libc++ -static-libstdc++)

vk_add_library(popular_common OBJECT ${POPULAR_COMMON_SOURCES} ${LIGHT_COMMON_SOURCES})
set_property(TARGET popular_common PROPERTY POSITION_INDEPENDENT_CODE ON)

