// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/profiler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <unordered_set>

#include "common/algorithms/compare.h"
#include "common/containers/final_action.h"
#include "common/cycleclock.h"
#include "common/wrappers/string_view.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/php_assert.h"
#include "server/php-queries.h"

namespace {
template<class T>
class TracingProfilerStack : vk::not_copyable {
public:
  T *update_top(T *new_head) noexcept {
    std::swap(top_, new_head);
    return new_head;
  }

  T *get_top() const noexcept {
    return top_;
  }

private:
  T *top_{nullptr};
};

class ProfilerContext : vk::not_copyable {
public:
  static ProfilerContext &get() noexcept {
    static ProfilerContext context;
    return context;
  }

  void global_init() noexcept {
    start_tp_ = std::chrono::steady_clock::now();
    start_tsc_ = cycleclock_now();
  }

  TracingProfilerStack<FunctionStatsNoLabel> function_list;
  TracingProfilerStack<ProfilerBase> call_stack;
  TracingProfilerStack<ResumableProfilerBase> current_resumable_call_stack;
  uint64_t start_waiting_tsc{0};
  uint64_t last_waiting_tsc{0};
  uint64_t waiting_generation{0};
  bool flush_required{false};
  char profiler_log_suffix[PATH_MAX]{'\0'};

  uint64_t get_memory_used_monotonic(const memory_resource::MemoryStats &memory_stats) noexcept {
    last_memory_used_ = std::max(last_memory_used_, uint64_t{memory_stats.memory_used});
    return last_memory_used_;
  }

  uint64_t get_real_memory_used_monotonic(const memory_resource::MemoryStats &memory_stats) noexcept {
    last_real_memory_used_ = std::max(last_real_memory_used_, uint64_t{memory_stats.real_memory_used});
    return last_real_memory_used_;
  }

  void reset() noexcept {
    php_assert(!call_stack.get_top());
    php_assert(!current_resumable_call_stack.get_top());
    start_waiting_tsc = 0;
    last_waiting_tsc = 0;
    waiting_generation = 0;
    flush_required = false;
    *profiler_log_suffix = '\0';
    last_memory_used_ = 0;
    last_real_memory_used_ = 0;
  }

  long double calc_nanoseconds_to_tsc_rate() const noexcept {
    if (!start_tsc_ || start_tp_ == std::chrono::steady_clock::time_point{}) {
      return 1.0;
    }
    const uint64_t interval_tsc = cycleclock_now() - start_tsc_;
    const std::chrono::nanoseconds interval_nanoseconds = std::chrono::steady_clock::now() - start_tp_;
    if (!interval_tsc || !interval_nanoseconds.count()) {
      return 1.0;
    }
    return static_cast<long double>(interval_nanoseconds.count()) / static_cast<long double>(interval_tsc);
  }

  void update_log_suffix(vk::string_view suffix) noexcept {
    *profiler_log_suffix = '.';
    size_t suffix_len = suffix.size();
    if (suffix_len) {
      memcpy(profiler_log_suffix + 1, suffix.data(), suffix_len);
      profiler_log_suffix[1 + suffix_len++] = '.';
    }
    profiler_log_suffix[1 + suffix_len] = '\0';
  }

private:
  ProfilerContext() = default;

  uint64_t last_memory_used_{0};
  uint64_t last_real_memory_used_{0};

  std::chrono::steady_clock::time_point start_tp_;
  uint64_t start_tsc_{0};
};

uint64_t tsc2ns(uint64_t tsc, long double nanoseconds_to_tsc_rate) noexcept {
  return static_cast<uint64_t>(static_cast<long double>(tsc) * nanoseconds_to_tsc_rate);
}

struct {
  const char *log_path_mask{nullptr};
} profiler_config;

template<typename F>
FunctionStatsBase::Stats &stats_binary_op(FunctionStatsBase::Stats &lhs, const FunctionStatsBase::Stats &rhs, const F &f) noexcept {
  f(lhs.total_allocations, rhs.total_allocations);
  f(lhs.total_memory_allocated, rhs.total_memory_allocated);
  f(lhs.memory_used, rhs.memory_used);
  f(lhs.real_memory_used, rhs.real_memory_used);
  f(lhs.working_tsc, rhs.working_tsc);
  f(lhs.waiting_tsc, rhs.waiting_tsc);
  f(lhs.context_swaps_for_query_count, rhs.context_swaps_for_query_count);
  f(lhs.rpc_requests_stat, rhs.rpc_requests_stat);
  f(lhs.sql_requests_stat, rhs.sql_requests_stat);
  f(lhs.mc_requests_stat, rhs.mc_requests_stat);
  return lhs;
}

} // namespace

FunctionStatsBase::Stats &FunctionStatsBase::Stats::operator+=(const Stats &other) noexcept {
  ++calls;
  return stats_binary_op(*this, other, [](auto &lhs, auto rhs) { lhs += rhs; });
}

FunctionStatsBase::Stats &FunctionStatsBase::Stats::operator-=(const Stats &other) noexcept {
  return stats_binary_op(*this, other, [](auto &lhs, auto rhs) { lhs -= rhs; });
}

void FunctionStatsBase::Stats::write_to(FILE *out, long double nanoseconds_to_tsc_rate) const noexcept {
  fprintf(out,
          "%" PRIu64 " %" PRIu64
          " %" PRIu64 " %" PRIu64
          " %" PRIu64
          " %" PRIu64 " %" PRIu64 " %" PRIu64
          " %" PRIu64 " %" PRIu64 " %" PRIu64
          " %" PRIu64 " %" PRIu64 " %" PRIu64
          " %" PRIu64 " %" PRIu64 "\n",
          total_allocations, total_memory_allocated,
          memory_used, real_memory_used,
          context_swaps_for_query_count,
          rpc_requests_stat.queries_count(), rpc_requests_stat.outgoing_bytes(), rpc_requests_stat.incoming_bytes(),
          sql_requests_stat.queries_count(), sql_requests_stat.outgoing_bytes(), sql_requests_stat.incoming_bytes(),
          mc_requests_stat.queries_count(), mc_requests_stat.outgoing_bytes(), mc_requests_stat.incoming_bytes(),
          tsc2ns(working_tsc, nanoseconds_to_tsc_rate),
          tsc2ns(waiting_tsc, nanoseconds_to_tsc_rate));
}

FunctionStatsBase::FunctionStatsBase(const char *file, const char *function, size_t line, size_t level) noexcept:
  file_name(file),
  function_name(function),
  function_line(line),
  profiler_level(level) {
}

void FunctionStatsBase::on_call_finish(const Stats &profiled_stats) noexcept {
  self_stats_ += profiled_stats;
}

void FunctionStatsBase::on_callee_call_finish(const FunctionStatsBase &callee_stats, const Stats &callee_profiled_stats) noexcept {
  self_stats_ -= callee_profiled_stats;
  dl::CriticalSectionGuard critical_section;
  callees_[&callee_stats] += callee_profiled_stats;
}

uint64_t FunctionStatsBase::calls_count() const noexcept {
  return self_stats_.calls;
}

void FunctionStatsBase::flush(FILE *out, long double nanoseconds_to_tsc_rate) noexcept {
  if (out) {
    fprintf(out, "\nfl=%s\nfn=", file_name);
    write_function_name_with_label(out);
    fprintf(out, "\n%zu ", function_line);
    self_stats_.write_to(out, nanoseconds_to_tsc_rate);

    for (const auto &callee: callees_) {
      fprintf(out, "cfl=%s\ncfn=", callee.first->file_name);
      callee.first->write_function_name_with_label(out);
      fprintf(out, "\ncalls=%" PRIu64 "\n%zu ", callee.second.calls, function_line);
      callee.second.write_to(out, nanoseconds_to_tsc_rate);
    }
  }

  self_stats_ = Stats{};
  callees_.clear();
}

FunctionStatsNoLabel::FunctionStatsNoLabel(const char *file, const char *function, size_t line, size_t level) noexcept:
  FunctionStatsBase(file, function, line, level),
  TracingProfilerListNode<FunctionStatsNoLabel>(ProfilerContext::get().function_list.update_top(this)) {
}

FunctionStatsWithLabel &FunctionStatsNoLabel::get_stats_with_label(vk::string_view label) noexcept {
  auto it = labels_.find(label);
  if (it != labels_.end()) {
    return *it->second;
  }
  dl::CriticalSectionGuard critical_section;
  auto label_ptr = std::make_unique<FunctionStatsWithLabel>(*this, label);
  // FunctionStatsWithLabel object owns raw string, therefore reassign the label
  label = vk::string_view{label_ptr->get_label(), label.size()};
  return *labels_.emplace(label, std::move(label_ptr)).first->second;
}

FunctionStatsWithLabel::FunctionStatsWithLabel(FunctionStatsNoLabel &no_label, vk::string_view label) noexcept:
  FunctionStatsBase(no_label.file_name, no_label.function_name, no_label.function_line, no_label.profiler_level),
  parent_stats_no_label(no_label) {
  php_assert(label.size() < label_buffer_.size());
  std::memcpy(label_buffer_.data(), label.data(), label.size());
  label_buffer_[label.size()] = '\0';
}

void ProfilerBase::set_initial_stats(uint64_t start_working_tsc, const memory_resource::MemoryStats &memory_stats) noexcept {
  start_working_tsc_ = start_working_tsc;
  start_total_allocations_ = memory_stats.total_allocations;
  start_total_memory_allocated_ = memory_stats.total_memory_allocated;
  auto &context = ProfilerContext::get();
  start_memory_used_ = context.get_memory_used_monotonic(memory_stats);
  start_real_memory_used_ = context.get_real_memory_used_monotonic(memory_stats);
  start_context_swaps_for_query_count_ = static_cast<uint64_t>(get_net_queries_count());
  start_rpc_queries_stat_ = PhpQueriesStats::get_rpc_queries_stat();
  start_sql_queries_stat_ = PhpQueriesStats::get_sql_queries_stat();
  start_mc_queries_stat_ = PhpQueriesStats::get_mc_queries_stat();
}

void ProfilerBase::start() noexcept {
  next_ = ProfilerContext::get().call_stack.update_top(this);
  set_initial_stats(cycleclock_now(), dl::get_script_memory_stats());
}

void ProfilerBase::stop() noexcept {
  FunctionStatsBase::Stats profiled_stats;
  profiled_stats.working_tsc = cycleclock_now() - start_working_tsc_;
  profiled_stats.waiting_tsc = accumulated_waiting_tsc_;
  const auto &memory_stats = dl::get_script_memory_stats();
  profiled_stats.total_allocations = memory_stats.total_allocations - start_total_allocations_;
  profiled_stats.total_memory_allocated = memory_stats.total_memory_allocated - start_total_memory_allocated_;
  auto &context = ProfilerContext::get();
  profiled_stats.memory_used = context.get_memory_used_monotonic(memory_stats) - start_memory_used_;
  profiled_stats.real_memory_used = context.get_real_memory_used_monotonic(memory_stats) - start_real_memory_used_;
  profiled_stats.context_swaps_for_query_count = static_cast<uint64_t>(get_net_queries_count()) - start_context_swaps_for_query_count_;
  profiled_stats.rpc_requests_stat = PhpQueriesStats::get_rpc_queries_stat() - start_rpc_queries_stat_;
  profiled_stats.sql_requests_stat = PhpQueriesStats::get_sql_queries_stat() - start_sql_queries_stat_;
  profiled_stats.mc_requests_stat = PhpQueriesStats::get_mc_queries_stat() - start_mc_queries_stat_;
  function_stats_->on_call_finish(profiled_stats);

  auto *caller = get_next();
  auto *self = context.call_stack.update_top(caller);
  php_assert(self == this);
  if (caller) {
    if (accumulated_waiting_tsc_) {
      php_assert(caller->last_callee_waiting_generation_ <= waiting_generation_);
      if (caller->last_callee_waiting_generation_ == waiting_generation_) {
        profiled_stats.waiting_tsc = 0;
      }
      caller->last_callee_waiting_generation_ = waiting_generation_;
    }
    caller->function_stats_->on_callee_call_finish(*function_stats_, profiled_stats);
  }
  accumulated_waiting_tsc_ = 0;
  context.flush_required = true;
}

void ProfilerBase::fix_waiting_tsc(uint64_t waiting_tsc) noexcept {
  start_working_tsc_ += waiting_tsc;
  accumulated_waiting_tsc_ += waiting_tsc;
  const uint64_t waiting_generation = ProfilerContext::get().waiting_generation;
  php_assert(waiting_generation_ < waiting_generation);
  waiting_generation_ = waiting_generation;
}

void ProfilerBase::use_function_label(vk::string_view label) noexcept {
  stop();
  function_stats_ = &function_stats_->get_stats_with_label(label);
  start();
}

bool ProfilerBase::is_profiling_allowed(bool is_root) noexcept {
  return profiler_config.log_path_mask &&
         (is_root || ProfilerContext::get().call_stack.get_top());
}

void ProfilerBase::forcibly_dump_log_on_finish(const char *function_name) noexcept {
  ProfilerContext &context = ProfilerContext::get();
  if (*context.profiler_log_suffix) {
    return;
  }

  std::array<char, 1024> suffix_buffer;
  size_t suffix_size = 0;
  if (function_name) {
    vk::string_view suffix{function_name};
    suffix_size = std::min(suffix.size(), suffix_buffer.size());
    std::replace_copy_if(suffix.begin(), suffix.begin() + suffix_size, suffix_buffer.begin(),
                         [](char c) { return !isalnum(c); }, '_');
  }
  context.update_log_suffix({suffix_buffer.data(), suffix_size});
}

bool ShutdownProfiler::is_root() noexcept {
  const FunctionStatsBase *any_function = ProfilerContext::get().function_list.get_top();
  return any_function && any_function->profiler_level == 2;
}

void ResumableProfilerBase::attach_resumable_caller() noexcept {
  php_assert(!resumable_caller_);
  php_assert(!resumable_sibling_callee_next_);
  php_assert(!resumable_sibling_callee_prev_);
  resumable_caller_ = ProfilerContext::get().current_resumable_call_stack.update_top(this);
  if (resumable_caller_) {
    resumable_sibling_callee_next_ = resumable_caller_->resumable_callee_;
    resumable_caller_->resumable_callee_ = this;
    if (resumable_sibling_callee_next_) {
      resumable_sibling_callee_next_->resumable_sibling_callee_prev_ = this;
    }
  }
}

void ResumableProfilerBase::detach_resumable_caller() noexcept {
  if (!resumable_caller_) {
    return;
  }
  if (resumable_caller_->resumable_callee_ == this) {
    resumable_caller_->resumable_callee_ = resumable_sibling_callee_next_;
    if (resumable_sibling_callee_next_) {
      resumable_sibling_callee_next_->resumable_sibling_callee_prev_ = nullptr;
    }
  } else {
    php_assert(resumable_sibling_callee_prev_);
    resumable_sibling_callee_prev_->resumable_sibling_callee_next_ = resumable_sibling_callee_next_;
    if (resumable_sibling_callee_next_) {
      resumable_sibling_callee_next_->resumable_sibling_callee_prev_ = resumable_sibling_callee_prev_;
    }
  }
  resumable_sibling_callee_prev_ = nullptr;
  resumable_sibling_callee_next_ = nullptr;
  resumable_caller_ = nullptr;
}

void ResumableProfilerBase::detach_resumable_callee() noexcept {
  ResumableProfilerBase *callee = resumable_callee_;
  while (callee) {
    ResumableProfilerBase *sibling_callee = callee->resumable_sibling_callee_next_;
    callee->resumable_sibling_callee_next_ = nullptr;
    callee->resumable_sibling_callee_prev_ = nullptr;
    callee->resumable_caller_ = nullptr;
    callee = sibling_callee;
  }
  resumable_callee_ = nullptr;
}

void ResumableProfilerBase::start_all_functions_from_this_resumable_stack() noexcept {
  auto &context = ProfilerContext::get();
  php_assert(!context.current_resumable_call_stack.get_top());

  const auto &memory_stats = dl::get_script_memory_stats();
  set_initial_stats(cycleclock_now(), memory_stats);
  ProfilerBase *caller = context.call_stack.update_top(this);

  php_assert(accumulated_waiting_tsc_ == 0);
  std::swap(accumulated_waiting_tsc_, context.last_waiting_tsc);
  if (accumulated_waiting_tsc_) {
    php_assert(waiting_generation_ < context.waiting_generation);
    waiting_generation_ = context.waiting_generation;
  }
  context.current_resumable_call_stack.update_top(this);
  ResumableProfilerBase *resumable_root = this;
  ResumableProfilerBase *resumable_caller = resumable_caller_;
  while (resumable_caller) {
    if (accumulated_waiting_tsc_) {
      php_assert(resumable_caller->waiting_generation_ < context.waiting_generation);
      resumable_caller->waiting_generation_ = context.waiting_generation;
    }
    resumable_caller->set_initial_stats(start_working_tsc_, memory_stats);
    resumable_caller->accumulated_waiting_tsc_ += accumulated_waiting_tsc_;
    resumable_root = resumable_caller;
    resumable_caller = resumable_caller->resumable_caller_;
  }
  resumable_root->set_next(caller);
}

void ResumableProfilerBase::stop_all_callers_from_this_resumable_stack() noexcept {
  ResumableProfilerBase *caller = resumable_caller_;
  while (caller) {
    caller->stop();
    caller = caller->resumable_caller_;
  }
  ProfilerContext::get().current_resumable_call_stack.update_top(nullptr);
}

void ResumableProfilerBase::start_resumable(bool resumable_awakening) noexcept {
  if (resumable_awakening) {
    awakening_call_ = true;
    start_all_functions_from_this_resumable_stack();
  } else {
    start();
    attach_resumable_caller();
  }
}

void ResumableProfilerBase::stop_resumable(bool resumable_finishing) noexcept {
  auto &resumable_call_stack = ProfilerContext::get().current_resumable_call_stack;
  ResumableProfilerBase *resumable_top = resumable_call_stack.get_top();
  php_assert(resumable_top == this);
  stop();

  if (awakening_call_) {
    stop_all_callers_from_this_resumable_stack();
  } else {
    resumable_call_stack.update_top(resumable_caller_);
  }

  if (resumable_finishing) {
    detach_resumable_callee();
    detach_resumable_caller();
    function_stats_ = nullptr;
  }
}

void notify_profiler_start_waiting() noexcept {
  auto &context = ProfilerContext::get();
  if (context.call_stack.get_top()) {
    php_assert(context.start_waiting_tsc == 0);
    context.start_waiting_tsc = cycleclock_now();
    context.last_waiting_tsc = 0;
  }
}

void notify_profiler_stop_waiting() noexcept {
  auto &context = ProfilerContext::get();
  if (ProfilerBase *top = context.call_stack.get_top()) {
    php_assert(context.start_waiting_tsc != 0);
    php_assert(context.last_waiting_tsc == 0);
    const uint64_t last_waiting_tsc = cycleclock_now() - context.start_waiting_tsc;
    context.start_waiting_tsc = 0;
    ++context.waiting_generation;
    do {
      top->fix_waiting_tsc(last_waiting_tsc);
      top = top->get_next();
    } while (top);
    if (!context.current_resumable_call_stack.get_top()) {
      context.last_waiting_tsc = last_waiting_tsc;
    }
  }
}

void forcibly_stop_profiler() noexcept {
  auto &context = ProfilerContext::get();
  if (context.start_waiting_tsc) {
    notify_profiler_stop_waiting();
  }

  ProfilerBase *top = context.call_stack.get_top();
  while (top) {
    ResumableProfilerBase *resumable_top = context.current_resumable_call_stack.get_top();
    if (resumable_top == top) {
      resumable_top->stop_resumable(true);
    } else {
      top->stop();
    }
    top = context.call_stack.get_top();
  }
  php_assert(!context.current_resumable_call_stack.get_top());
}

bool set_profiler_log_path(const char *mask) noexcept {
  size_t mask_len = strlen(mask);
  // reserve 128 bytes for pid + timestamp
  if (!mask_len || mask_len + 128 > PATH_MAX) {
    return false;
  }
  profiler_config.log_path_mask = mask;
  return true;
}

void global_init_profiler() noexcept {
  ProfilerContext::get().global_init();
}

void forcibly_stop_and_flush_profiler() noexcept {
  if (!profiler_config.log_path_mask) {
    return;
  }

  forcibly_stop_profiler();
  auto &context = ProfilerContext::get();
  auto context_reset = vk::finally([&context] { context.reset(); });
  if (!context.flush_required) {
    return;
  }

  FunctionStatsNoLabel *next_function = context.function_list.get_top();
  if (!*context.profiler_log_suffix) {
    while (next_function) {
      next_function->flush();
      for (const auto &function_with_label : next_function->get_labels()) {
        function_with_label.second->flush();
      }
      next_function = next_function->get_next();
    }
    return;
  }

  char profiler_log_path[PATH_MAX * 2]{'\0'};
  const int len = snprintf(profiler_log_path, sizeof(profiler_log_path) - 1,
                           "%s%s%" PRIX64 ".%d",
                           profiler_config.log_path_mask, context.profiler_log_suffix,
                           static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()), getpid());
  php_assert(len > 0 && sizeof(profiler_log_path) >= static_cast<size_t>(len + 1));
  FILE *profiler_log = fopen(profiler_log_path, "w");
  fprintf(profiler_log,
          "# callgrind format\n"
          "event: total_allocations : Memory: Total allocations count\n"
          "event: total_memory_allocated : Memory: Total allocated bytes\n"
          "event: memory_allocated : Memory: Extra allocated bytes\n"
          "event: real_memory_allocated : Memory: Extra real allocated bytes\n"

          "event: query_ctx_swaps : Queries: Context swaps for queries\n"

          "event: rpc_outgoing_queries: Queries: RPC outgoing queries\n"
          "event: rpc_outgoing_traffic: Traffic: RPC outgoing bytes\n"
          "event: rpc_incoming_traffic: Traffic: RPC incoming bytes\n"

          "event: sql_outgoing_queries: Queries: SQL outgoing queries\n"
          "event: sql_outgoing_traffic: Traffic: SQL outgoing bytes\n"
          "event: sql_incoming_traffic: Traffic: SQL incoming bytes\n"

          "event: mc_outgoing_queries: Queries: MC outgoing queries\n"
          "event: mc_outgoing_traffic: Traffic: MC outgoing bytes\n"
          "event: mc_incoming_traffic: Traffic: MC incoming bytes\n"

          "event: summary_queries = rpc_outgoing_queries + sql_outgoing_queries + mc_outgoing_queries: Queries: Summary outgoing queries\n"
          "event: summary_outgoing_traffic = rpc_outgoing_traffic + sql_outgoing_traffic + mc_outgoing_traffic: Traffic: Summary outgoing bytes\n"
          "event: summary_incoming_traffic = rpc_incoming_traffic + sql_incoming_traffic + mc_incoming_traffic: Traffic: Summary incoming bytes\n"

          "event: cpu_ns : Nanoseconds: CPU\n"
          "event: net_ns : Nanoseconds: Net\n"
          "event: total_ns = cpu_ns + net_ns : Nanoseconds: Full\n"

          "events: "
          "total_allocations total_memory_allocated memory_allocated real_memory_allocated "
          "query_ctx_swaps "
          "rpc_outgoing_queries rpc_outgoing_traffic rpc_incoming_traffic "
          "sql_outgoing_queries sql_outgoing_traffic sql_incoming_traffic "
          "mc_outgoing_queries mc_outgoing_traffic mc_incoming_traffic "
          "cpu_ns net_ns\n\n");

  const long double nanoseconds_to_tsc_rate = context.calc_nanoseconds_to_tsc_rate();
  next_function = context.function_list.get_top();
  while (next_function) {
    if (next_function->calls_count()) {
      next_function->flush(profiler_log, nanoseconds_to_tsc_rate);
    }
    for (const auto &function_with_label : next_function->get_labels()) {
      if (function_with_label.second->calls_count()) {
        function_with_label.second->flush(profiler_log, nanoseconds_to_tsc_rate);
      }
    }
    next_function = next_function->get_next();
  }
  fclose(profiler_log);
}

void f$profiler_set_log_suffix(const string &suffix) noexcept {
  // +2 for 2x. and +1 for \0
  if (suffix.size() + 3 > PATH_MAX) {
    php_warning("Trying to set too long suffix '%s'", suffix.c_str());
    return;
  }
  for (string::size_type i = 0; i != suffix.size(); ++i) {
    if (unlikely(!isalnum(suffix[i]) && suffix[i] != '_')) {
      php_warning("Trying to set suffix '%s' with non alphanumeric character", suffix.c_str());
      return;
    }
  }

  ProfilerContext::get().update_log_suffix({suffix.c_str(), suffix.size()});
}

void f$profiler_set_function_label(const string &label) noexcept {
  auto &context = ProfilerContext::get();
  if (ProfilerBase *top = context.call_stack.get_top()) {
    vk::string_view label_view{label.c_str(), label.size()};
    if (unlikely(label_view.empty())) {
      php_warning("Trying to set an empty label, it will be ignored");
      return;
    }
    if (unlikely(vk::any_of(label_view, ::iscntrl))) {
      php_warning("Trying to set label '%s' with bad character, the label will be ignored", label.c_str());
      return;
    }

    if (unlikely(label_view.size() > FunctionStatsWithLabel::label_max_len())) {
      php_warning("Trying to set too long label '%s', it will be truncated to '%.*s'",
                  label.c_str(), static_cast<int>(FunctionStatsWithLabel::label_max_len()), label.c_str());
      label_view.remove_suffix(label.size() - FunctionStatsWithLabel::label_max_len());
    }
    top->use_function_label(label_view);
  }
}
