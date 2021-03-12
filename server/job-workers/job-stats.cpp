// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <new>
#include <sys/mman.h>

#include "server/job-workers/job-stats.h"

namespace job_workers {

JobStats &JobStats::make() {
  void *shared_mem = mmap(nullptr, sizeof(JobStats), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  auto *ctx = new (shared_mem) JobStats{};
  return *ctx;
}

JobStats &JobStats::get() {
  static JobStats &ctx = make();
  return ctx;
}

} // namespace job_workers
