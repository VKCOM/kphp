// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cinttypes>
#include <cstdio>
#include <unordered_map>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime/allocator.h"
#include "server/php-queries-stats.h"

template<class T>
class TracingProfilerListNode : vk::not_copyable {
public:
  explicit TracingProfilerListNode(T *next = nullptr) :
    next_(next) {
  }

  T *get_next() const noexcept {
    return next_;
  }

  T *set_next(T *new_next) noexcept {
    std::swap(new_next, next_);
    return new_next;
  }

protected:
  T *next_{nullptr};
};

class FunctionStatsWithLabel;

class FunctionStatsBase {
public:
  struct Stats {
    uint64_t calls{0};
    uint64_t total_allocations{0};
    uint64_t total_memory_allocated{0};
    uint64_t memory_used{0};
    uint64_t real_memory_used{0};
    uint64_t working_tsc{0};
    uint64_t waiting_tsc{0};
    uint64_t context_swaps_for_query_count{0};
    QueriesStat rpc_requests_stat;
    QueriesStat sql_requests_stat;
    QueriesStat mc_requests_stat;

    Stats &operator+=(const Stats &other) noexcept;
    Stats &operator-=(const Stats &other) noexcept;
    void write_to(FILE *out, long double nanoseconds_to_tsc_rate) const noexcept;
  };

  FunctionStatsBase(const char *file, const char *function, size_t line, size_t level) noexcept;

  void on_call_finish(const Stats &profiled_stats) noexcept;

  void on_callee_call_finish(const FunctionStatsBase &callee_stats, const Stats &callee_profiled_stats) noexcept;

  uint64_t calls_count() const noexcept;

  virtual FunctionStatsWithLabel &get_stats_with_label(vk::string_view label) noexcept = 0;
  virtual void write_function_name_with_label(FILE *out) const noexcept = 0;

  void flush(FILE *out = nullptr, long double nanoseconds_to_tsc_rate = 1.0) noexcept;

  const char *const file_name{nullptr};
  const char *const function_name{nullptr};

  const size_t function_line{0};
  const size_t profiler_level{0};

protected:
  virtual ~FunctionStatsBase() = default;

  Stats self_stats_;
  std::unordered_map<const FunctionStatsBase *, Stats> callees_;
};


class FunctionStatsNoLabel final : public FunctionStatsBase, public TracingProfilerListNode<FunctionStatsNoLabel> {
public:
  FunctionStatsNoLabel(const char *file, const char *function, size_t line, size_t level) noexcept;

  const auto &get_labels() const noexcept {
    return labels_;
  }

  void write_function_name_with_label(FILE *out) const noexcept final {
    fprintf(out, "%s", function_name);
  }

  FunctionStatsWithLabel &get_stats_with_label(vk::string_view label) noexcept final;

private:
  std::unordered_map<vk::string_view, std::unique_ptr<FunctionStatsWithLabel>> labels_;
};

class FunctionStatsWithLabel final : public FunctionStatsBase {
public:
  FunctionStatsWithLabel(FunctionStatsNoLabel &no_label, vk::string_view label) noexcept;

  const char *get_label() const noexcept {
    return label_buffer_.data();
  }

  static constexpr size_t label_max_len() noexcept {
    return label_max_len_;
  }

  void write_function_name_with_label(FILE *out) const noexcept final {
    fprintf(out, "%s (%s)", function_name, label_buffer_.data());
  }

  FunctionStatsWithLabel &get_stats_with_label(vk::string_view label) noexcept final {
    return parent_stats_no_label.get_stats_with_label(label);
  }

  FunctionStatsNoLabel &parent_stats_no_label;

private:
  static constexpr size_t label_max_len_ = 255;
  std::array<char, label_max_len_ + 1> label_buffer_;
};

class ProfilerBase : public TracingProfilerListNode<ProfilerBase> {
public:
  void start() noexcept;
  void stop() noexcept;
  void fix_waiting_tsc(uint64_t waiting_tsc) noexcept;
  void use_function_label(vk::string_view label) noexcept;

protected:
  ProfilerBase() = default;
  ~ProfilerBase() = default;

  void set_initial_stats(uint64_t start_working_tsc, const memory_resource::MemoryStats &memory_stats) noexcept;

  static void forcibly_dump_log_on_finish(const char *function_name) noexcept;

  static bool is_profiling_allowed(bool is_root) noexcept;

  template<class FunctionTag>
  static FunctionStatsNoLabel &get_function_stats() noexcept {
    static FunctionStatsNoLabel function_stats{
      FunctionTag::file_name(),
      FunctionTag::function_name(),
      FunctionTag::function_line(),
      FunctionTag::profiler_level()
    };
    if (FunctionTag::is_root()) {
      forcibly_dump_log_on_finish(FunctionTag::profiler_level() == 1 ? FunctionTag::function_name() : nullptr);
    }
    return function_stats;
  }

  uint64_t start_total_allocations_{0};
  uint64_t start_total_memory_allocated_{0};
  uint64_t start_memory_used_{0};
  uint64_t start_real_memory_used_{0};
  uint64_t start_working_tsc_{0};
  uint64_t accumulated_waiting_tsc_{0};
  uint64_t start_context_swaps_for_query_count_{0};

  QueriesStat start_rpc_queries_stat_;
  QueriesStat start_sql_queries_stat_;
  QueriesStat start_mc_queries_stat_;

  uint64_t last_callee_waiting_generation_{0};
  uint64_t waiting_generation_{0};
  FunctionStatsBase *function_stats_{nullptr};
};

template<class FunctionTag>
class AutoProfiler : ProfilerBase {
public:
  AutoProfiler() noexcept {
    if (is_profiling_allowed(FunctionTag::is_root())) {
      function_stats_ = &get_function_stats<FunctionTag>();
      start();
    }
  }

  ~AutoProfiler() noexcept {
    if (function_stats_) {
      stop();
      function_stats_ = nullptr;
    }
  }
};

class ShutdownProfiler final : AutoProfiler<ShutdownProfiler> {
public:
  static constexpr const char *file_name() noexcept { return "(unknown)"; }
  static constexpr const char *function_name() noexcept { return "<shutdown caller>"; }
  static constexpr size_t function_line() noexcept { return 0; }
  static constexpr size_t profiler_level() noexcept { return 2; }
  static bool is_root() noexcept;
};

class ResumableProfilerBase : public ProfilerBase {
public:
  void start_resumable(bool resumable_awakening) noexcept;
  void stop_resumable(bool resumable_finishing) noexcept;

protected:
  ~ResumableProfilerBase() = default;

private:
  void attach_resumable_caller() noexcept;
  void detach_resumable_caller() noexcept;

  void detach_resumable_callee() noexcept;

  void start_all_functions_from_this_resumable_stack() noexcept;
  void stop_all_callers_from_this_resumable_stack() noexcept;

  ResumableProfilerBase *resumable_caller_{nullptr};

  // Pointer to a beginning of bidirectional list of called functions (below)
  ResumableProfilerBase *resumable_callee_{nullptr};

  // A bidirectional list of called sibling functions (pointer to a beginning is stored in caller)
  ResumableProfilerBase *resumable_sibling_callee_next_{nullptr};
  ResumableProfilerBase *resumable_sibling_callee_prev_{nullptr};

  bool awakening_call_{false};
};

template<class FunctionTag>
class ResumableProfiler final : ResumableProfilerBase {
public:
  ResumableProfiler() noexcept {
    if (is_profiling_allowed(FunctionTag::is_root())) {
      function_stats_ = &get_function_stats<FunctionTag>();
    }
  }

  void start(bool resumable_awakening) noexcept {
    if (function_stats_) {
      ResumableProfilerBase::start_resumable(resumable_awakening);
    }
  }

  void stop(bool resumable_finishing) noexcept {
    if (function_stats_) {
      ResumableProfilerBase::stop_resumable(resumable_finishing);
    }
  }
};

void notify_profiler_start_waiting() noexcept;
void notify_profiler_stop_waiting() noexcept;

void forcibly_stop_profiler() noexcept;
void forcibly_stop_and_flush_profiler() noexcept;

bool set_profiler_log_path(const char *mask) noexcept;

void global_init_profiler() noexcept;

void f$profiler_set_log_suffix(const string &suffix) noexcept;
void f$profiler_set_function_label(const string &label) noexcept;
