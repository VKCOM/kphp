#include "server/shared-data-worker-cache.h"

#include "server/server-stats.h"

void SharedDataWorkerCache::init_defaults() noexcept {
  uint16_t total_workers = vk::singleton<WorkersControl>::get().get_count(WorkerType::general_worker);
  uint16_t running_workers = vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::general_worker);
  cached_worker_stats = {running_workers, 0, static_cast<uint16_t>(total_workers - running_workers), total_workers};
}

void SharedDataWorkerCache::on_worker_cron() noexcept {
  const auto now = std::chrono::steady_clock::now();
  if (now - last_update_ >= std::chrono::seconds{1}) {
    cached_worker_stats = vk::singleton<SharedData>::get().load_worker_stats();
    last_update_ = now;
  }
}

const WorkersStats &SharedDataWorkerCache::get_cached_worker_stats() const noexcept {
  return cached_worker_stats;
}
