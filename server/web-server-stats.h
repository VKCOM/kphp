#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/stats/provider.h"

#include "server/server-stats.h"

struct WebServerStats : vk::not_copyable {
  friend class vk::singleton<WebServerStats>;

  using Stats = std::tuple<int64_t, int64_t, int64_t, int64_t>;

  void init() noexcept;

  void init_default_stats() noexcept;

  void store(ServerStats::WorkersStat const & stats) noexcept;

  void sync_this_worker_stats() noexcept;

  WebServerStats::Stats load() const noexcept;

private:
  WebServerStats() = default;

  struct Storage {
    std::atomic<uint16_t> running_workers{0};
    std::atomic<uint16_t> waiting_workers{0};
    std::atomic<uint16_t> ready_for_accept_workers{0};
    std::atomic<uint16_t> total_workers{0};
  };

  std::chrono::steady_clock::time_point last_update_;
  Storage *webserver_stats{nullptr};
  Stats buffered_webserver_stats;
};
