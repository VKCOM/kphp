// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kfs/kfs-internal.h"

#include <assert.h>
#include <cstdio>
#include <string.h>

#include "common/kprintf.h"
#include "common/wrappers/likely.h"

/*
 * This file contains functions which are only used inside kfs but in more than one implementation file
 * and thus cannot be static.
 */

void _buffer_crypt(kfs_replica_handle_t R, unsigned char *buff, long long size, unsigned char iv[16], long long off) {
  assert(R);
  assert(R->ctx_crypto);
  if (unlikely(size < 0)) {
    kprintf("size = %lld\n", size);
    assert(size >= 0);
  }
  if (!size) {
    return;
  }
  while (true) {
    int w = 0x7ffffff0 < size ? 0x7ffffff0 : size;
    R->ctx_crypto->ctr_crypt(R->ctx_crypto, buff, buff, w, iv, off);
    size -= w;
    if (!size) {
      break;
    }
    buff += w;
    off += w;
  }
}
