// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_KFS_INTERNAL_H
#define ENGINE_KFS_INTERNAL_H

#include <assert.h>
#include <cstdlib>

#include "common/kfs/kfs-common.h"

/*
 * This file is only intended to be included by kfs implementation source files.
 */

void _kfs_file_info_decref(struct kfs_file_info *FI);

void _buffer_crypt(kfs_replica_handle_t R, unsigned char *buff, long long size, unsigned char iv[16], long long off);

#endif // ENGINE_KFS_INTERNAL_H
