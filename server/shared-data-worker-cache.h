#pragma once

#include <chrono>
#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/stats/provider.h"
#include "server/shared-data.h"

/**
 * Shared data cache that's updated in cron in each worker.
 */
class SharedDataWorkerCache : vk::not_copyable {
public:
  void init_defaults() noexcept;

  void on_worker_cron() noexcept;

  const WorkersStats &get_cached_worker_stats() const noexcept;

private:
  std::chrono::steady_clock::time_point last_update_;
  WorkersStats cached_worker_stats;

  SharedDataWorkerCache() = default;

  friend vk::singleton<SharedDataWorkerCache>;
};
