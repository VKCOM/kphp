// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <new>
#include <sys/mman.h>

#include "server/task-workers/shared-context.h"

SharedContext &SharedContext::get() {
  assert(inited);
  static void *shared_mem = mmap(nullptr, sizeof(SharedContext), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  static auto *ctx = new(shared_mem) SharedContext;
  return *ctx;
}

bool SharedContext::inited = false;

void SharedContext::init() {
  inited = true;
  get();
}
