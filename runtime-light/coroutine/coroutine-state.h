// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::coro {

// NOTE: this is one-off profiling instrumentation for sizing the future task<T> stack
// allocator/object pool (branch kkotliar/k2_count_coroutine_stats). Safe to remove wholesale.
struct chain_stats {
  size_t task_count{};     // cumulative task<T> allocations during this chain's life
  size_t task_bytes{};     // cumulative bytes for task<T> allocations during this chain's life
  size_t task_depth{};     // current live task<T> depth (running counter)
  size_t task_depth_max{}; // max depth reached during this chain's life
};

struct coro_profile_stats {
  size_t chains_total{};
  size_t chains_finished{};
  size_t chains_concurrent{};
  size_t chains_concurrent_max{};
  bool concurrency_tracking_started{};
  uint64_t concurrency_window_start_ns{};
  uint64_t concurrency_last_ts_ns{};
  uint64_t concurrency_time_weighted_ns{}; // sum(concurrency_level * elapsed_ns)

  size_t task_frame_alloc_count{};
  size_t task_frame_bytes_total{};
  size_t task_frame_bytes_max{};
  size_t task_bytes_live{}; // current live bytes, instance-wide
  size_t task_bytes_live_peak{};

  size_t chain_task_count_sum{};
  size_t chain_task_count_max{};
  size_t chain_task_bytes_sum{};
  size_t chain_task_bytes_max{};
  size_t chain_task_depth_sum{};
  size_t chain_task_depth_max{};
};

struct instance_state final : private vk::not_copyable {

  instance_state() noexcept = default;

  static instance_state& get() noexcept;

  kphp::coro::async_stack_root coroutine_stack_root;

  // ambient "which chain is running right now" pointer, mirrors top_async_stack_frame
  chain_stats* current_chain_stats{};
  coro_profile_stats profile_stats;

  void record_task_frame_alloc(size_t n) noexcept {
    auto& stats{profile_stats};
    ++stats.task_frame_alloc_count;
    stats.task_frame_bytes_total += n;
    stats.task_frame_bytes_max = std::max(stats.task_frame_bytes_max, n);
    stats.task_bytes_live += n;
    stats.task_bytes_live_peak = std::max(stats.task_bytes_live_peak, stats.task_bytes_live);

    if (current_chain_stats != nullptr) {
      auto& chain{*current_chain_stats};
      ++chain.task_count;
      chain.task_bytes += n;
      ++chain.task_depth;
      chain.task_depth_max = std::max(chain.task_depth_max, chain.task_depth);
    }
  }

  void record_task_frame_free(size_t n) noexcept {
    profile_stats.task_bytes_live -= n;

    if (current_chain_stats != nullptr) {
      --current_chain_stats->task_depth;
    }
  }

  void note_chain_started() noexcept {
    auto& stats{profile_stats};
    ++stats.chains_total;
    update_concurrency_accumulator();
    ++stats.chains_concurrent;
    stats.chains_concurrent_max = std::max(stats.chains_concurrent_max, stats.chains_concurrent);
  }

  void note_chain_finished(const chain_stats& finished_chain) noexcept {
    auto& stats{profile_stats};
    update_concurrency_accumulator();
    --stats.chains_concurrent;

    ++stats.chains_finished;
    stats.chain_task_count_sum += finished_chain.task_count;
    stats.chain_task_count_max = std::max(stats.chain_task_count_max, finished_chain.task_count);
    stats.chain_task_bytes_sum += finished_chain.task_bytes;
    stats.chain_task_bytes_max = std::max(stats.chain_task_bytes_max, finished_chain.task_bytes);
    stats.chain_task_depth_sum += finished_chain.task_depth_max;
    stats.chain_task_depth_max = std::max(stats.chain_task_depth_max, finished_chain.task_depth_max);
  }

private:
  void update_concurrency_accumulator() noexcept {
    auto& stats{profile_stats};
    k2::TimePoint now{};
    k2::instant(std::addressof(now));

    if (!stats.concurrency_tracking_started) {
      stats.concurrency_tracking_started = true;
      stats.concurrency_window_start_ns = now.time_point_ns;
      stats.concurrency_last_ts_ns = now.time_point_ns;
      return;
    }

    stats.concurrency_time_weighted_ns += (now.time_point_ns - stats.concurrency_last_ts_ns) * stats.chains_concurrent;
    stats.concurrency_last_ts_ns = now.time_point_ns;
  }
};

} // namespace kphp::coro
