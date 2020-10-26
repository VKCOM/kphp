#ifndef ENGINE_KFS_BINLOG_H
#define ENGINE_KFS_BINLOG_H

#include <stdbool.h>

#include "common/kfs/kfs-typedefs.h"

kfs_file_handle_t open_binlog(const kfs_replica_t *Replica, long long log_pos);
kfs_file_handle_t next_binlog(kfs_file_handle_t log_handle);

int close_binlog(kfs_file_handle_t F, bool close_handle);

#endif // ENGINE_KFS_BINLOG_H
