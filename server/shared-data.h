#pragma once

#include <atomic>
#include <tuple>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

struct WorkersStats {
  uint16_t running_workers{0};
  uint16_t waiting_workers{0};
  uint16_t ready_for_accept_workers{0};
  uint16_t total_workers{0};
};

/**
 * Shared data between master and workers.
 */
class SharedData : vk::not_copyable {
  struct Storage {
    std::atomic<WorkersStats> workers_stats;
  };
  static_assert(sizeof(WorkersStats) <= 8, "It's used with std::atomic");
public:
  void init();

  void store_worker_stats(const WorkersStats &workers_stats) noexcept {
    storage->workers_stats.store(workers_stats, std::memory_order_relaxed);
  };

  WorkersStats load_worker_stats() noexcept {
    return storage->workers_stats.load(std::memory_order_relaxed);
  };
private:
  Storage *storage;

  SharedData() = default;

  friend vk::singleton<SharedData>;
};
