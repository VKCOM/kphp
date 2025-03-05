prepend(COMMON_TESTS_SOURCES ${COMMON_DIR}/
        algorithms/compare-test.cpp
        algorithms/contains-test.cpp
        algorithms/hashes-test.cpp
        algorithms/projections-test.cpp
        algorithms/simd-int-to-string-test.cpp
        algorithms/string-algorithms-test.cpp
        allocators/freelist-test.cpp
        allocators/lockfree-slab-test.cpp
        crc32c-test.cpp
        crypto/aes256-test.cpp
        parallel/counter-test.cpp
        parallel/limit-counter-test.cpp
        parallel/maximum-test.cpp
        smart_iterators/smart-iterators-test.cpp
        smart_ptrs/tagged-ptr-test.cpp
        type_traits/list_of_types_test.cpp
        wrappers/span-test.cpp
        wrappers/string_view-test.cpp
        ucontext/ucontext-portable-test.cpp)

allow_deprecated_declarations(${COMMON_TESTS_SOURCES}/algorithms/projections-test.cpp)

set(COMMON_TESTS_LIBS vk::${PIC_MODE}::common-src vk::${PIC_MODE}::net-src vk::${PIC_MODE}::binlog-src vk::${PIC_MODE}::unicode ${EPOLL_SHIM_LIB} OpenSSL::${PIC_MODE}::Crypto ZSTD::${PIC_MODE}::zstd ZLIB::${PIC_MODE}::zlib)
vk_add_unittest(common "${COMMON_TESTS_LIBS}" ${COMMON_TESTS_SOURCES})
