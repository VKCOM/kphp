#include "server/web-server-stats.h"

#include "server/server-stats.h"

void WebServerStats::init_default_stats() noexcept {
  uint16_t total_workers = vk::singleton<WorkersControl>::get().get_count(WorkerType::general_worker);
  uint16_t running_workers = vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::general_worker);
  buffered_webserver_stats = WebServerStats::Stats(running_workers, 0, total_workers - running_workers, total_workers);
}

void WebServerStats::store(WebServerStats::Stats const &stats) noexcept {
  auto &shared_part = vk::singleton<DataSharing>::get().acquire<SharedPart>();
  shared_part.running_workers.store(std::get<0>(stats), std::memory_order_relaxed);
  shared_part.waiting_workers.store(std::get<1>(stats), std::memory_order_relaxed);
  shared_part.ready_for_accept_workers.store(std::get<2>(stats), std::memory_order_relaxed);
  shared_part.total_workers.store(std::get<3>(stats), std::memory_order_relaxed);
}

void WebServerStats::sync_this_worker_stats() noexcept {
  const auto now = std::chrono::steady_clock::now();
  if (now - last_update_ >= std::chrono::seconds{1}) {
    const auto &shared_part = vk::singleton<DataSharing>::get().acquire<SharedPart>();
    buffered_webserver_stats =
      std::tuple(shared_part.running_workers.load(std::memory_order_relaxed), shared_part.waiting_workers.load(std::memory_order_relaxed),
                 shared_part.ready_for_accept_workers.load(std::memory_order_relaxed), shared_part.total_workers.load(std::memory_order_relaxed));
    last_update_ = now;
  }
}

WebServerStats::Stats WebServerStats::get_cache() const noexcept {
  return buffered_webserver_stats;
}
