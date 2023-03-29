#include "web-server-stats.h"

#include "common/wrappers/memory-utils.h"

void WebServerStats::init() noexcept {
  webserver_stats = new (mmap_shared(sizeof(WebServerStats))) WebServerStats::Storage();
}

void WebServerStats::init_default_stats() noexcept {
  uint16_t total_workers = vk::singleton<WorkersControl>::get().get_count(WorkerType::general_worker);
  uint16_t running_workers = vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::general_worker);
  buffered_webserver_stats = WebServerStats::Stats(running_workers,0,total_workers - running_workers,total_workers);
}

void WebServerStats::store(ServerStats::WorkersStat const &stats) noexcept {
  assert(webserver_stats != nullptr);
  webserver_stats->running_workers.store(stats.running_workers, std::memory_order_relaxed);
  webserver_stats->waiting_workers.store(stats.waiting_workers, std::memory_order_relaxed);
  webserver_stats->ready_for_accept_workers.store(stats.ready_for_accept_workers, std::memory_order_relaxed);
  webserver_stats->total_workers.store(stats.total_workers, std::memory_order_relaxed);
}

void WebServerStats::sync_this_worker_stats() noexcept {
  assert(webserver_stats != nullptr);
  const auto now = std::chrono::steady_clock::now();
  if (now - last_update_ >= std::chrono::seconds{1}) {
    buffered_webserver_stats = std::tuple(
      webserver_stats->running_workers.load(std::memory_order_relaxed), webserver_stats->waiting_workers.load(std::memory_order_relaxed),
      webserver_stats->ready_for_accept_workers.load(std::memory_order_relaxed), webserver_stats->total_workers.load(std::memory_order_relaxed));
    last_update_ = now;
  }
}

WebServerStats::Stats WebServerStats::load() const noexcept {
  return buffered_webserver_stats;
}
