// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <chrono>
#include <tuple>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

struct WorkersStats {
  using PackerRepr = uint64_t;

  uint16_t running_workers{0};
  uint16_t waiting_workers{0};
  uint16_t ready_for_accept_workers{0};
  uint16_t total_workers{0};

  void unpack(PackerRepr stats) noexcept;
  PackerRepr pack() const noexcept;
};

/**
 * Shared data between master and workers.
 */
class SharedData : vk::not_copyable {
  struct Storage {
    std::atomic<WorkersStats::PackerRepr> workers_stats;
    std::atomic<uint64_t> start_use_host_in_statshouse_metrics_timestamp{0};
  };

public:
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;

  void init();

  void store_worker_stats(const WorkersStats &workers_stats) noexcept {
    storage->workers_stats.store(workers_stats.pack(), std::memory_order_relaxed);
  };

  WorkersStats load_worker_stats() noexcept {
    WorkersStats workers_stats;
    workers_stats.unpack(storage->workers_stats.load(std::memory_order_relaxed));
    return workers_stats;
  };

  void store_start_use_host_in_statshouse_metrics_timestamp(const time_point &tp) noexcept {
    storage->start_use_host_in_statshouse_metrics_timestamp.store(tp.time_since_epoch().count(), std::memory_order_release);
  }

  time_point load_start_use_host_in_statshouse_metrics_timestamp() noexcept {
    auto t = storage->start_use_host_in_statshouse_metrics_timestamp.load(std::memory_order_acquire);
    return time_point{clock::duration{t}};
  }
private:
  Storage *storage;

  SharedData() = default;

  friend vk::singleton<SharedData>;
};
