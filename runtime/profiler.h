#pragma once

#include <cinttypes>
#include <cstdio>
#include <utility>
#include <unordered_map>

#include "common/mixin/not_copyable.h"

#include "runtime/kphp_core.h"

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

class FunctionStats : public TracingProfilerListNode<FunctionStats> {
public:
  FunctionStats(const char *file, const char *function, size_t line, size_t level) noexcept;

  void on_call_finish(uint64_t total_allocations, uint64_t total_memory_allocated,
                      uint64_t memory_used, uint64_t real_memory_used,
                      uint64_t working_tsc, uint64_t waiting_tsc) noexcept;

  void on_callee_call_finish(const FunctionStats &callee_stats,
                             uint64_t callee_total_allocations, uint64_t callee_total_memory_allocated,
                             uint64_t callee_memory_used, uint64_t callee_real_memory_used,
                             uint64_t callee_working_tsc, uint64_t callee_waiting_tsc) noexcept;

  uint64_t calls_count() const noexcept;

  void flush(FILE *out = nullptr) noexcept;

  const char *const file_name{nullptr};
  const uint64_t file_id{0};

  const char *const function_name{nullptr};
  const uint64_t function_id{0};

  const size_t function_line{0};
  const size_t profiler_level{0};

private:
  uint64_t calls_{0};
  uint64_t self_total_allocations_{0};
  uint64_t self_total_memory_allocated_{0};
  uint64_t self_memory_used_{0};
  uint64_t self_real_memory_used_{0};
  uint64_t self_working_tsc_{0};
  uint64_t self_waiting_tsc_{0};

  struct CalleeStats {
    uint64_t calls{0};
    uint64_t total_allocations{0};
    uint64_t total_memory_allocated{0};
    uint64_t memory_used{0};
    uint64_t real_memory_used{0};
    uint64_t working_tsc{0};
    uint64_t waiting_tsc{0};
  };

  std::unordered_map<const FunctionStats *, CalleeStats> callees_;
};

class ProfilerBase : public TracingProfilerListNode<ProfilerBase> {
public:
  void start() noexcept;
  void stop() noexcept;
  void fix_waiting_tsc(uint64_t waiting_tsc) noexcept;

protected:
  ProfilerBase() = default;
  ~ProfilerBase() = default;

  void set_initial_stats(uint64_t start_working_tsc, const memory_resource::MemoryStats &memory_stats) noexcept;

  static bool is_profiling_allowed(bool is_root) noexcept;

  template<class FunctionTag>
  static FunctionStats *get_function_stats() noexcept {
    static FunctionStats function_stats{
      FunctionTag::file_name(),
      FunctionTag::function_name(),
      FunctionTag::function_line(),
      FunctionTag::profiler_level()
    };
    return &function_stats;
  }

  uint64_t start_total_allocations_{0};
  uint64_t start_total_memory_allocated_{0};
  uint64_t start_memory_used_{0};
  uint64_t start_real_memory_used_{0};
  uint64_t start_working_tsc_{0};
  uint64_t accumulated_waiting_tsc_{0};
  uint64_t last_callee_waiting_generation_{0};
  uint64_t waiting_generation_{0};
  FunctionStats *function_stats_{nullptr};
};

template<class FunctionTag>
class AutoProfiler : ProfilerBase {
public:
  AutoProfiler() noexcept {
    if (is_profiling_allowed(FunctionTag::is_root())) {
      function_stats_ = get_function_stats<FunctionTag>();
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

class ShutdownProfiler : AutoProfiler<ShutdownProfiler> {
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

  // Указатель на начало двусвязного вызываемых фукнций (ниже)
  ResumableProfilerBase *resumable_callee_{nullptr};

  // Двусвязный список вызываемых фукнций - братьев (указатель на начало находится в caller)
  ResumableProfilerBase *resumable_sibling_callee_next_{nullptr};
  ResumableProfilerBase *resumable_sibling_callee_prev_{nullptr};

  bool awakening_call_{false};
};

template<class FunctionTag>
class ResumableProfiler : ResumableProfilerBase {
public:
  ResumableProfiler() noexcept {
    if (is_profiling_allowed(FunctionTag::is_root())) {
      function_stats_ = get_function_stats<FunctionTag>();
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

void f$dump_profiler_log_on_finish(const string &suffix = {}) noexcept;
