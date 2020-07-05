#include "runtime/profiler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <unordered_set>

#include "common/cycleclock.h"
#include "common/wrappers/string_view.h"
#include "common/containers/final_action.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/php_assert.h"

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

  TracingProfilerStack<FunctionStats> function_list;
  TracingProfilerStack<ProfilerBase> call_stack;
  TracingProfilerStack<ResumableProfilerBase> current_resumable_call_stack;
  uint64_t start_waiting_tsc{0};
  uint64_t last_waiting_tsc{0};
  uint64_t waiting_generation{0};
  bool flush_required{false};
  char profiler_log_suffix[PATH_MAX]{'\0'};

  uint64_t get_memory_used_monotonic(const memory_resource::MemoryStats &memory_stats) noexcept {
    last_memory_used_ = std::max(last_memory_used_, memory_stats.memory_used);
    return last_memory_used_;
  }

  uint64_t get_real_memory_used_monotonic(const memory_resource::MemoryStats &memory_stats) noexcept {
    last_real_memory_used_ = std::max(last_real_memory_used_, memory_stats.real_memory_used);
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

  uint64_t get_next_function_id() noexcept {
    return ++function_ids_counter_;
  }

  uint64_t get_next_file_id(vk::string_view file_name) noexcept {
    dl::CriticalSectionGuard critical_section;
    if (unlikely(!file_ids_)) {
      // Специально делаем ленивую инициализацию,
      // чтобы конструктор синглтона был максимально тривиальным и не генерировал лишний код
      file_ids_ = new(&file_ids_storage) File2IdMap();
    }
    return file_ids_->emplace(file_name, file_ids_->size() + 1).first->second;
  }

  const auto &get_file_ids() const noexcept {
    php_assert(file_ids_);
    return *file_ids_;
  }

private:
  ProfilerContext() = default;

  uint64_t last_memory_used_{0};
  uint64_t last_real_memory_used_{0};

  uint64_t function_ids_counter_{0};

  using File2IdMap = std::unordered_map<vk::string_view, uint64_t>;
  File2IdMap *file_ids_{nullptr};
  std::aligned_storage_t<sizeof(File2IdMap), alignof(File2IdMap)> file_ids_storage{};
};

struct {
  const char *log_path_mask{nullptr};
} profiler_config;

} // namespace

FunctionStats::FunctionStats(const char *file, const char *function, size_t line, size_t level) noexcept:
  TracingProfilerListNode<FunctionStats>(ProfilerContext::get().function_list.update_top(this)),
  file_name(file),
  file_id(ProfilerContext::get().get_next_file_id(file)),
  function_name(function),
  function_id(ProfilerContext::get().get_next_function_id()),
  function_line(line),
  profiler_level(level) {
}

void FunctionStats::on_call_finish(uint64_t total_allocations, uint64_t total_memory_allocated,
                                   uint64_t memory_used, uint64_t real_memory_used,
                                   uint64_t working_tsc, uint64_t waiting_tsc) noexcept {
  ++calls_;
  self_total_allocations_ += total_allocations;
  self_total_memory_allocated_ += total_memory_allocated;
  self_memory_used_ += memory_used;
  self_real_memory_used_ += real_memory_used;
  self_working_tsc_ += working_tsc;
  self_waiting_tsc_ += waiting_tsc;
}

void FunctionStats::on_callee_call_finish(const FunctionStats &callee_stats,
                                          uint64_t callee_total_allocations, uint64_t callee_total_memory_allocated,
                                          uint64_t callee_memory_used, uint64_t callee_real_memory_used,
                                          uint64_t callee_working_tsc, uint64_t callee_waiting_tsc) noexcept {
  self_total_allocations_ -= callee_total_allocations;
  self_total_memory_allocated_ -= callee_total_memory_allocated;
  self_memory_used_ -= callee_memory_used;
  self_real_memory_used_ -= callee_real_memory_used;
  self_working_tsc_ -= callee_working_tsc;
  self_waiting_tsc_ -= callee_waiting_tsc;
  dl::CriticalSectionGuard critical_section;
  auto &callee = callees_[&callee_stats];
  ++callee.calls;
  callee.total_allocations += callee_total_allocations;
  callee.total_memory_allocated += callee_total_memory_allocated;
  callee.memory_used += callee_memory_used;
  callee.real_memory_used += callee_real_memory_used;
  callee.working_tsc += callee_working_tsc;
  callee.waiting_tsc += callee_waiting_tsc;
}

uint64_t FunctionStats::calls_count() const noexcept {
  return calls_;
}

void FunctionStats::flush(FILE *out) noexcept {
  if (out) {
    fprintf(out,
            "\n"
            "fl=(%" PRIu64 ")\n"
            "fn=(%" PRIu64 ")\n"
            "%zu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
            file_id, function_id, function_line,
            self_total_allocations_, self_total_memory_allocated_,
            self_memory_used_, self_real_memory_used_,
            self_working_tsc_, self_waiting_tsc_
    );

    for (const auto &callee: callees_) {
      fprintf(out,
              "cfl=(%" PRIu64 ")\n"
              "cfn=(%" PRIu64 ")\n"
              "calls=%zu\n"
              "%zu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
              callee.first->file_id, callee.first->function_id, callee.second.calls, function_line,
              callee.second.total_allocations, callee.second.total_memory_allocated,
              callee.second.memory_used, callee.second.real_memory_used,
              callee.second.working_tsc, callee.second.waiting_tsc
      );
    }
  }

  calls_ = 0;
  self_total_allocations_ = 0;
  self_total_memory_allocated_ = 0;
  self_memory_used_ = 0;
  self_real_memory_used_ = 0;
  self_working_tsc_ = 0;
  self_waiting_tsc_ = 0;
  callees_.clear();
}

void ProfilerBase::set_initial_stats(uint64_t start_working_tsc, const memory_resource::MemoryStats &memory_stats) noexcept {
  start_working_tsc_ = start_working_tsc;
  start_total_allocations_ = memory_stats.total_allocations;
  start_total_memory_allocated_ = memory_stats.total_memory_allocated;
  auto &context = ProfilerContext::get();
  start_memory_used_ = context.get_memory_used_monotonic(memory_stats);
  start_real_memory_used_ = context.get_real_memory_used_monotonic(memory_stats);
}

void ProfilerBase::start() noexcept {
  next_ = ProfilerContext::get().call_stack.update_top(this);
  set_initial_stats(cycleclock_now(), dl::get_script_memory_stats());
}

void ProfilerBase::stop() noexcept {
  const uint64_t working_tsc = cycleclock_now() - start_working_tsc_;
  const auto &memory_stats = dl::get_script_memory_stats();
  const uint64_t total_allocations = memory_stats.total_allocations - start_total_allocations_;
  const uint64_t total_memory_allocated = memory_stats.total_memory_allocated - start_total_memory_allocated_;
  auto &context = ProfilerContext::get();
  const uint64_t memory_used = context.get_memory_used_monotonic(memory_stats) - start_memory_used_;
  const uint64_t real_memory_used = context.get_real_memory_used_monotonic(memory_stats) - start_real_memory_used_;
  function_stats_->on_call_finish(total_allocations, total_memory_allocated,
                                  memory_used, real_memory_used,
                                  working_tsc, accumulated_waiting_tsc_);

  auto *caller = get_next();
  auto *self = context.call_stack.update_top(caller);
  php_assert(self == this);
  if (caller) {
    uint64_t waiting_tsc_for_caller = accumulated_waiting_tsc_;
    if (accumulated_waiting_tsc_) {
      php_assert(caller->last_callee_waiting_generation_ <= waiting_generation_);
      if (caller->last_callee_waiting_generation_ == waiting_generation_) {
        waiting_tsc_for_caller = 0;
      }
      caller->last_callee_waiting_generation_ = waiting_generation_;
    }
    caller->function_stats_->on_callee_call_finish(*function_stats_,
                                                   total_allocations, total_memory_allocated,
                                                   memory_used, real_memory_used,
                                                   working_tsc, waiting_tsc_for_caller);
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

bool ProfilerBase::is_profiling_allowed(bool is_root) noexcept {
  return profiler_config.log_path_mask &&
         (is_root || ProfilerContext::get().call_stack.get_top());
}

bool ShutdownProfiler::is_root() noexcept {
  const FunctionStats *any_function = ProfilerContext::get().function_list.get_top();
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
  // 128 зарезервированно для pid + timestamp
  if (!mask_len || mask_len + 128 > PATH_MAX) {
    return false;
  }
  profiler_config.log_path_mask = mask;
  return true;
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

  FunctionStats *next_function = context.function_list.get_top();
  if (!*context.profiler_log_suffix) {
    while (next_function) {
      next_function->flush();
      next_function = next_function->get_next();
    }
    return;
  }

  char profiler_log_path[PATH_MAX * 2]{'\0'};
  const int len = snprintf(profiler_log_path, sizeof(profiler_log_path) - 1,
                           "%s%s%" PRIX64 ".%d",
                           profiler_config.log_path_mask, context.profiler_log_suffix,
                           std::chrono::system_clock::now().time_since_epoch().count(), getpid());
  php_assert(len > 0 && sizeof(profiler_log_path) >= static_cast<size_t>(len + 1));
  FILE *profiler_log = fopen(profiler_log_path, "w");
  fprintf(profiler_log,
          "# callgrind format\n"
          "event: total_allocations : Memory: total allocations count\n"
          "event: total_memory_allocated : Memory: total allocated bytes\n"
          "event: memory_allocated : Memory: extra allocated bytes\n"
          "event: real_memory_allocated : Memory: extra real allocated bytes\n"
          "event: cpu_tsc : TSC: CPU\n"
          "event: net_tsc : TSC: Net\n"
          "event: total_tsc = cpu_tsc + net_tsc : TSC: Full\n"
          "events: total_allocations total_memory_allocated memory_allocated real_memory_allocated cpu_tsc net_tsc\n\n");

  fprintf(profiler_log, "# define file ID mapping\n");
  for (const auto &file2id: context.get_file_ids()) {
    fprintf(profiler_log, "fl=(%" PRIu64 ") %.*s\n",
            file2id.second, static_cast<int>(file2id.first.size()), file2id.first.data());
  }

  fprintf(profiler_log, "\n# define function ID mapping\n");
  while (next_function) {
    if (next_function->calls_count()) {
      fprintf(profiler_log, "fn=(%" PRIu64 ") %s\n", next_function->function_id, next_function->function_name);
    }
    next_function = next_function->get_next();
  }

  next_function = context.function_list.get_top();
  while (next_function) {
    if (next_function->calls_count()) {
      next_function->flush(profiler_log);
    }
    next_function = next_function->get_next();
  }
  fclose(profiler_log);
}

void f$dump_profiler_log_on_finish(const string &suffix) noexcept {
  // +2 для 2x. и +1 для \0
  if (suffix.size() + 3 > PATH_MAX) {
    php_warning("Trying to set too long suffix '%s'", suffix.c_str());
    return;
  }
  for (string::size_type i = 0; i != suffix.size(); ++i) {
    if (!isalnum(suffix[i])) {
      php_warning("Trying to set suffix '%s' with non alphanumeric character", suffix.c_str());
      return;
    }
  }

  ProfilerContext &context = ProfilerContext::get();
  *context.profiler_log_suffix = '.';
  size_t suffix_len = suffix.size();
  if (suffix_len) {
    memcpy(context.profiler_log_suffix + 1, suffix.c_str(), suffix_len);
    context.profiler_log_suffix[1 + suffix_len++] = '.';
  }
  context.profiler_log_suffix[1 + suffix_len] = '\0';
}
