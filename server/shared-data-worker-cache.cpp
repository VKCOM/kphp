// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/shared-data-worker-cache.h"

#include "server/server-stats.h"

void SharedDataWorkerCache::init_defaults() noexcept {
  uint16_t total_workers = vk::singleton<WorkersControl>::get().get_count(WorkerType::general_worker);
  uint16_t running_workers = vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::general_worker);
  cached_worker_stats = {running_workers, 0, static_cast<uint16_t>(total_workers - running_workers), total_workers};
}

void SharedDataWorkerCache::on_worker_cron() noexcept {
  cached_worker_stats = vk::singleton<SharedData>::get().load_worker_stats();
}

const WorkersStats &SharedDataWorkerCache::get_cached_worker_stats() const noexcept {
  return cached_worker_stats;
}
