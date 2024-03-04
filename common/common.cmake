prepend(COMMON_MAIN_SOURCES ${COMMON_DIR}/
        pipe-utils.cpp
        pid.cpp
        dl-utils-lite.cpp
        server/stats.cpp
        server/statsd-client.cpp
        server/init-binlog.cpp
        server/init-snapshot.cpp

        sha1.cpp
        allocators/freelist.cpp
        md5.cpp
        allocators/lockfree-slab.cpp
        server/main-binlog.cpp
        crypto/aes256.cpp
        crypto/aes256-generic.cpp
        crypto/aes256-${CMAKE_SYSTEM_PROCESSOR}.cpp

        fast-backtrace.cpp
        string-processing.cpp
        kphp-tasks-lease/lease-worker-mode.cpp)

prepend(COMMON_KFS_SOURCES ${COMMON_DIR}/kfs/
        kfs.cpp
        kfs-internal.cpp
        kfs-binlog.cpp
        kfs-snapshot.cpp
        kfs-replica.cpp)

prepend(COMMON_TL_SOURCES ${COMMON_DIR}/tl/
        parse.cpp
        query-header.cpp)

prepend(COMMON_TL_METHODS_SOURCES ${COMMON_DIR}/tl/methods/
        rwm.cpp
        string.cpp)

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    prepend(COMMON_UCONTEXT_SOURCES ${COMMON_DIR}/ucontext/
            ucontext-arm.cpp)
endif()

set(COMMON_ALL_SOURCES
    ${COMMON_MAIN_SOURCES}
    ${COMMON_KFS_SOURCES}
    ${COMMON_TL_METHODS_SOURCES}
    ${COMMON_TL_SOURCES}
    ${COMMON_UCONTEXT_SOURCES})

if(COMPILER_CLANG)
    set_source_files_properties(${COMMON_DIR}/string-processing.cpp PROPERTIES COMPILE_FLAGS -Wno-invalid-source-encoding)
endif()

vk_add_library(common_src OBJECT ${COMMON_ALL_SOURCES})
