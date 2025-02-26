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

set(COMMON_TESTS_LIBS common-src-no-pic net-src-no-pic binlog-src-no-pic unicode-no-pic ${EPOLL_SHIM_LIB} OpenSSL::no-pic::Crypto ZSTD::no-pic::zstd ZLIB::no-pic::zlib)
vk_add_unittest(common "${COMMON_TESTS_LIBS}" ${COMMON_TESTS_SOURCES})
