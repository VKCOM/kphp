prepend(BINLOG_SOURCES ${COMMON_DIR}/binlog/
        kdb-binlog-common.cpp
        binlog-buffer.cpp
        binlog-buffer-aio.cpp
        binlog-buffer-rotation-points.cpp
        binlog-buffer-replay.cpp
        binlog-snapshot.cpp)

vk_add_library_no_pic(binlog-src-no-pic OBJECT ${BINLOG_SOURCES})
vk_add_library_pic(binlog-src-pic OBJECT ${BINLOG_SOURCES})