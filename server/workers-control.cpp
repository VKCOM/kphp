// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cmath>

#include "server/workers-control.h"

bool WorkersControl::init() noexcept {
  static_assert(static_cast<size_t>(WorkerType::types_count) == 2, "check yourself");

  auto &job_workers_meta = meta_[static_cast<size_t>(WorkerType::job_worker)];
  if (job_workers_meta.ratio > 0) {
    job_workers_meta.count = static_cast<uint16_t>(std::ceil(job_workers_meta.ratio * total_workers_count_));
    if (job_workers_meta.count >= total_workers_count_) {
      return false;
    }
  }
  meta_[static_cast<size_t>(WorkerType::general_worker)].count = total_workers_count_ - job_workers_meta.count;
  return true;
}

void WorkersControl::on_worker_terminating(WorkerType worker_type) noexcept {
  auto &meta = meta_[static_cast<size_t>(worker_type)];
  assert(meta.running);
  --meta.running;
  meta.dying++;
}

void WorkersControl::on_worker_removing(WorkerType worker_type, bool dying) noexcept {
  auto &meta = meta_[static_cast<size_t>(worker_type)];
  const auto before = dying ? meta.dying-- : meta.running--;
  assert(before);
}
