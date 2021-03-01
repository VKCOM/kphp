// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <new>
#include <sys/mman.h>

#include "server/job-workers/shared-context.h"

namespace job_workers {

SharedContext &SharedContext::make() {
  void *shared_mem = mmap(nullptr, sizeof(SharedContext), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  auto *ctx = new (shared_mem) SharedContext{};
  return *ctx;
}

SharedContext &SharedContext::get() {
  static SharedContext &ctx = make();
  return ctx;
}

} // namespace job_workers
