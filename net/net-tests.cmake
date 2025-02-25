prepend(NET_TESTS_SOURCES ${BASE_DIR}/net/
        net-aes-keys-test.cpp
        net-msg-test.cpp
        net-test.cpp
        time-slice-test.cpp)

set(NET_TESTS_LIBS common-src-no-pic net-src-no-pic binlog-src-no-pic unicode-no-pic ${NET_TESTS_LIBS} ${EPOLL_SHIM_LIB} OpenSSL::no-pic::Crypto ZSTD::no-pic::zstd ZLIB::no-pic::zlib)
vk_add_unittest(net "${NET_TESTS_LIBS}" ${NET_TESTS_SOURCES})
