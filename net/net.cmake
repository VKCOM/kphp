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

allow_deprecated_declarations_for_apple(${BASE_DIR}/net/net-crypto-aes.cpp ${BASE_DIR}/net/net-aes-keys.cpp)
vk_add_library(net_src OBJECT ${NET_SOURCES})
