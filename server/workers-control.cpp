// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>

#include "server/workers-control.h"

namespace {

constexpr uint16_t EMPTY_ID = std::numeric_limits<uint16_t>::max();

} // namespace

bool WorkersControl::init() noexcept {
  static_assert(static_cast<size_t>(WorkerType::types_count) == 2, "check yourself");

  auto &job_workers = meta_[static_cast<size_t>(WorkerType::job_worker)];
  if (job_workers.ratio > 0) {
    job_workers.count = static_cast<uint16_t>(std::ceil(job_workers.ratio * total_workers_count_));
    if (job_workers.count >= total_workers_count_) {
      return false;
    }
  }

  auto &general_workers = meta_[static_cast<size_t>(WorkerType::general_worker)];
  general_workers.count = total_workers_count_ - job_workers.count;

  general_workers.unique_ids_.fill(EMPTY_ID);
  std::iota(general_workers.unique_ids_.begin(), general_workers.unique_ids_.begin() + general_workers.count, uint16_t{0});

  job_workers.unique_ids_.fill(EMPTY_ID);
  std::iota(job_workers.unique_ids_.begin(), job_workers.unique_ids_.begin() + job_workers.count, general_workers.count);
  return true;
}

void WorkersControl::on_worker_terminating(WorkerType worker_type) noexcept {
  auto &meta = meta_[static_cast<size_t>(worker_type)];
  assert(meta.running);
  --meta.running;
  meta.dying++;
}

void WorkersControl::on_worker_removing(WorkerType worker_type, bool dying, uint16_t worker_unique_id) noexcept {
  auto &meta = meta_[static_cast<size_t>(worker_type)];
  const auto before = dying ? meta.dying-- : meta.running--;
  assert(before);
  const uint16_t alive = get_alive_count(worker_type);
  assert(alive < meta.count);
  const uint16_t next_unique_id = std::exchange(meta.unique_ids_[meta.count - alive - 1], worker_unique_id);
  assert(next_unique_id == EMPTY_ID);
}

uint16_t WorkersControl::on_worker_creating(WorkerType worker_type) noexcept {
  auto &meta = meta_[static_cast<size_t>(worker_type)];
  const uint16_t alive = get_alive_count(worker_type);
  assert(alive < meta.count);
  const uint16_t next_unique_id = std::exchange(meta.unique_ids_[meta.count - alive - 1], EMPTY_ID);
  assert(next_unique_id != EMPTY_ID);
  ++meta.running;
  return next_unique_id;
}
