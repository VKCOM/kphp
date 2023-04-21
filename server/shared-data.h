// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
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
  };

public:
  void init();

  void store_worker_stats(const WorkersStats &workers_stats) noexcept {
    storage->workers_stats.store(workers_stats.pack(), std::memory_order_relaxed);
  };

  WorkersStats load_worker_stats() noexcept {
    WorkersStats workers_stats;
    workers_stats.unpack(storage->workers_stats.load(std::memory_order_relaxed));
    return workers_stats;
  };
private:
  Storage *storage;

  SharedData() = default;

  friend vk::singleton<SharedData>;
};
