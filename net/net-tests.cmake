prepend(NET_TESTS_SOURCES ${BASE_DIR}/net/
        net-aes-keys-test.cpp
        net-msg-test.cpp
        net-test.cpp
        time-slice-test.cpp)

set(NET_TESTS_LIBS vk::${PIC_MODE}::common-src vk::${PIC_MODE}::net-src vk::${PIC_MODE}::binlog-src vk::${PIC_MODE}::unicode ${NET_TESTS_LIBS} ${EPOLL_SHIM_LIB} OpenSSL::${PIC_MODE}::Crypto ZSTD::${PIC_MODE}::zstd ZLIB::${PIC_MODE}::zlib)
vk_add_unittest(net "${NET_TESTS_LIBS}" ${NET_TESTS_SOURCES})
