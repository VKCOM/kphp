// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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
  WorkersStats cached_worker_stats;

  SharedDataWorkerCache() = default;

  friend vk::singleton<SharedDataWorkerCache>;
};
