include_guard(GLOBAL)

prepend(LIGHT_COMMON_SOURCES ${COMMON_DIR}/
        algorithms/simd-int-to-string.cpp
        resolver.cpp
        kprintf.cpp
        precise-time.cpp
        cpuid.cpp
        crc32.cpp
        crc32c.cpp
        options.cpp
        kernel-version.cpp
        secure-bzero.cpp
        crc32_${CMAKE_SYSTEM_PROCESSOR}.cpp
        crc32c_${CMAKE_SYSTEM_PROCESSOR}.cpp
        parallel/counter.cpp
        parallel/maximum.cpp
        parallel/thread-id.cpp
        parallel/limit-counter.cpp
        version-string.cpp
        rpc-headers.cpp
)

prepend(POPULAR_COMMON_SOURCES ${COMMON_DIR}/
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
vk_add_library(popular_common OBJECT ${POPULAR_COMMON_SOURCES} ${LIGHT_COMMON_SOURCES})
set_property(TARGET popular_common PROPERTY POSITION_INDEPENDENT_CODE ON)
