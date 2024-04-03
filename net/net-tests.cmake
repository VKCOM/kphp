prepend(NET_TESTS_SOURCES ${BASE_DIR}/net/
        net-aes-keys-test.cpp
        net-msg-test.cpp
        net-test.cpp
        time-slice-test.cpp)

prepare_cross_platform_libs(NET_TESTS_LIBS zstd)
set(NET_TESTS_LIBS vk::common_src vk::net_src vk::binlog_src vk::unicode ${NET_TESTS_LIBS} ${EPOLL_SHIM_LIB} OpenSSL::Crypto z)
vk_add_unittest(net "${NET_TESTS_LIBS}" ${NET_TESTS_SOURCES})
