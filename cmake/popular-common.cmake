include_guard(GLOBAL)

prepend(POPULAR_COMMON_SOURCES ${COMMON_DIR}/
        server/limits.cpp
        server/signals.cpp
        server/relogin.cpp
        server/crash-dump.cpp
        server/engine-settings.cpp
        stats/buffer.cpp
        stats/provider.cpp
        resolver.cpp
        kprintf.cpp
        precise-time.cpp
        cpuid.cpp
        crc32.cpp
        crc32c.cpp
        options.cpp
        kernel-version.cpp
        secure-bzero.cpp
        crc32_${HOST}.cpp
        crc32c_${HOST}.cpp
        parallel/counter.cpp
        parallel/maximum.cpp
        parallel/thread-id.cpp
        parallel/limit-counter.cpp
        version-string.cpp)

if(APPLE)
    list(APPEND POPULAR_COMMON_SOURCES ${COMMON_DIR}/macos-ports.cpp)
endif()

vk_add_library(popular_common OBJECT ${POPULAR_COMMON_SOURCES})
