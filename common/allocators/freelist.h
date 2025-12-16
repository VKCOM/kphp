// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#include "common/smart_ptrs/tagged-ptr.h"

// NB!: sizeof freelist object must be not less then 8 bytes

struct freelist {
  tagged_ptr_t pool;
};
typedef struct freelist freelist_t;

void freelist_init(freelist_t* freelist);
void* freelist_get(freelist_t* freelist);
bool freelist_try_put(freelist_t* freelist, void* ptr);
void freelist_put(freelist_t* freelist, void* ptr);
