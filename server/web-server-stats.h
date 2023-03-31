#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/stats/provider.h"
#include "server/data-sharing.h"
#include "server/server-stats.h"

namespace SharedData {
struct WebServerStatsBuffer : vk::not_copyable {
  std::atomic<uint16_t> running_workers{0};
  std::atomic<uint16_t> waiting_workers{0};
  std::atomic<uint16_t> ready_for_accept_workers{0};
  std::atomic<uint16_t> total_workers{0};
};
} // namespace SharedData

struct WebServerStats : vk::not_copyable {
  using Stats = std::tuple<int64_t, int64_t, int64_t, int64_t>;
  using SharedPart = SharedData::WebServerStatsBuffer;
  friend vk::singleton<WebServerStats>;

  void init_default_stats() noexcept;

  void store(ServerStats::WorkersStat const &stats) noexcept;

  void sync_this_worker_stats() noexcept;

  WebServerStats::Stats get() const noexcept;

private:
  WebServerStats() = default;
  std::chrono::steady_clock::time_point last_update_;
  Stats buffered_webserver_stats;
};

WebServerStats::Stats f$get_webserver_stats();
