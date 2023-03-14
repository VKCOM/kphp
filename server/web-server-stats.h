#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/stats/provider.h"

#include "server/server-stats.h"

struct WebServerStats : vk::not_copyable {
  using Stats = std::tuple<int64_t, int64_t, int64_t, int64_t>;

  void init() noexcept;

  bool store(ServerStats::WorkersStat const & stats);

  WebServerStats::Stats load() const;

private:
  struct Storage {
    std::atomic<uint16_t> running_workers{0};
    std::atomic<uint16_t> waiting_workers{0};
    std::atomic<uint16_t> ready_for_accept_workers{0};
    std::atomic<uint16_t> total_workers{0};
  };

  Storage *engine_stats_{nullptr};
};

extern WebServerStats::Stats webserver_stats;
