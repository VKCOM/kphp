prepend(BINLOG_SOURCES ${COMMON_DIR}/binlog/
        kdb-binlog-common.cpp
        binlog-buffer.cpp
        binlog-buffer-aio.cpp
        binlog-buffer-rotation-points.cpp
        binlog-buffer-replay.cpp)

vk_add_library(binlog_src OBJECT ${BINLOG_SOURCES})
