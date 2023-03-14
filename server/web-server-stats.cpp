#include "web-server-stats.h"

#include "common/wrappers/memory-utils.h"

void WebServerStats::init() noexcept {
  auto *workers_info = new (mmap_shared(sizeof(WebServerStats))) WebServerStats::Storage();
  engine_stats_ = workers_info;
}

bool WebServerStats::store(ServerStats::WorkersStat const & stats) {
  assert(engine_stats_ != nullptr);
  engine_stats_->running_workers.store(stats.running_workers, std::memory_order_relaxed);
  engine_stats_->waiting_workers.store(stats.waiting_workers, std::memory_order_relaxed);
  engine_stats_->ready_for_accept_workers.store(stats.ready_for_accept_workers, std::memory_order_relaxed);
  engine_stats_->total_workers.store(stats.total_workers, std::memory_order_relaxed);
  return true;
}

WebServerStats::Stats WebServerStats::load() const {
  assert(engine_stats_ != nullptr);
  return std::tuple(engine_stats_->running_workers.load(std::memory_order_relaxed),
                    engine_stats_->waiting_workers.load(std::memory_order_relaxed),
                    engine_stats_->ready_for_accept_workers.load(std::memory_order_relaxed),
                    engine_stats_->total_workers.load(std::memory_order_relaxed));
}

WebServerStats::Stats webserver_stats;
