prepend(BINLOG_SOURCES ${COMMON_DIR}/binlog/
        kdb-binlog-common.cpp
        binlog-buffer.cpp
        binlog-buffer-aio.cpp
        binlog-buffer-rotation-points.cpp
        binlog-buffer-replay.cpp
        binlog-snapshot.cpp)

vk_add_library_no_pic(binlog-src-no-pic OBJECT ${BINLOG_SOURCES})
add_dependencies(binlog-src-no-pic OpenSSL::no-pic::Crypto)
target_include_directories(binlog-src-no-pic PUBLIC ${OPENSSL_NO_PIC_INCLUDE_DIR})

vk_add_library_pic(binlog-src-pic OBJECT ${BINLOG_SOURCES})
add_dependencies(binlog-src-pic OpenSSL::pic::Crypto)
target_include_directories(binlog-src-pic PUBLIC ${OPENSSL_PIC_INCLUDE_DIR})
