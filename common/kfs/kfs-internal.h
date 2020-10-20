#ifndef ENGINE_KFS_INTERNAL_H
#define ENGINE_KFS_INTERNAL_H

#include <assert.h>
#include <malloc.h>

#include "common/kfs/kfs-common.h"

/*
 * This file is only intended to be included by kfs implementation source files.
 */

void _kfs_file_info_decref(struct kfs_file_info *FI);

void _buffer_crypt(kfs_replica_handle_t R, unsigned char *buff, long long size, unsigned char iv[16], long long off);

#endif // ENGINE_KFS_INTERNAL_H
