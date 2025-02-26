prepend(NET_SOURCES ${BASE_DIR}/net/
        net-buffers.cpp
        net-connections.cpp
        net-crypto-aes.cpp
        net-events.cpp
        net-memcache-server.cpp
        net-sockaddr-storage.cpp
        net-tcp-connections.cpp
        net-tcp-rpc-client.cpp
        net-tcp-rpc-common.cpp
        net-tcp-rpc-server.cpp
        net-ifnet.cpp
        net-socket-options.cpp
        net-dc.cpp
        net-aes-keys.cpp
        net-socket.cpp
        net-reactor.cpp
        net-msg-part.cpp
        net-mysql-client.cpp
        net-memcache-client.cpp
        net-http-server.cpp
        net-msg-buffers.cpp
        net-msg.cpp
        net-msg-part.cpp)

vk_add_library_no_pic(net-src-no-pic OBJECT ${NET_SOURCES})
vk_add_library_pic(net-src-pic OBJECT ${NET_SOURCES})
