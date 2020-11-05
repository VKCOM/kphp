// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_KFS_ENGINE_INIT_H
#define ENGINE_KFS_ENGINE_INIT_H

#include <stdbool.h>

#include "common/kfs/kfs-typedefs.h"

#define KFS_OPEN_REPLICA_FLAG_FORCE            1
#define KFS_OPEN_REPLICA_FLAG_IGNORE_ENCFILE   2
#define KFS_OPEN_REPLICA_OPEN_TMP_REPL_BINLOGS 4

kfs_replica_handle_t open_replica(const char *replica_name, int flags);
int update_replica(kfs_replica_handle_t R, int flags);
int close_replica(kfs_replica_handle_t R);

#endif // ENGINE_KFS_ENGINE_INIT_H
