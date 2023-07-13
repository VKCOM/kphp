// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-runner.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "common/fast-backtrace.h"
#include "common/kernel-version.h"
#include "common/kprintf.h"
#include "common/wrappers/memory-utils.h"
#include "net/net-connections.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/curl.h"
#include "runtime/exception.h"
#include "runtime/interface.h"
#include "runtime/kphp_tracing.h"
#include "runtime/oom_handler.h"
#include "runtime/profiler.h"
#include "server/json-logger.h"
#include "server/php-engine-vars.h"
#include "server/php-queries.h"
#include "server/server-log.h"
#include "server/server-stats.h"
#include "server/signal-handlers.h"

query_stats_t query_stats;
long long query_stats_id = 1;

namespace {
//TODO: sometimes I need to call old handlers
//TODO: recheck!

[[maybe_unused]] const void *main_thread_stack = nullptr;
[[maybe_unused]] size_t main_thread_stacksize = 0;
} // namespace

void PhpScript::error(const char *error_message, script_error_t error_type) noexcept {
  assert (in_script_context == true);
  in_script_context = false;
  current_script->state = run_state_t::error;
  current_script->error_message = error_message;
  current_script->error_type = error_type;
  stack_end = reinterpret_cast<char *>(exit_context.uc_stack.ss_sp) + exit_context.uc_stack.ss_size;
#if ASAN_ENABLED
  __sanitizer_start_switch_fiber(nullptr, main_thread_stack, main_thread_stacksize);
#endif
  setcontext_portable(&exit_context);
}

void PhpScript::try_run_shutdown_functions_on_timeout() noexcept {
  // The delayed timeout check is performed in the following cases
  // [1] context swap
  // [2] start_resumable
  // TODO add try_run_shutdown_functions_on_timeout in some buildin functions
  php_assert(PhpScript::in_script_context);
  if (!time_limit_exceeded) {
    return;
  }
  if (vk::singleton<OomHandler>::get().is_running()) {
    perform_error_if_running("timeout exit in OOM handler\n", script_error_t::timeout);
    return;
  }

  if (dl::is_malloc_replaced()) {
    dl::rollback_malloc_replacement();
  }

  if (get_shutdown_functions_count() != 0 && get_shutdown_functions_status() == shutdown_functions_status::not_executed) {
    run_shutdown_functions_from_timeout();
  }
  perform_error_if_running("timeout exit\n", script_error_t::timeout);
}

void PhpScript::check_net_context_errors() noexcept {
  php_assert(PhpScript::in_script_context);
  if (memory_limit_exceeded) {
    vk::singleton<OomHandler>::get().invoke();
    perform_error_if_running("memory limit exit\n", script_error_t::memory_limit);
  }
  try_run_shutdown_functions_on_timeout();
}

PhpScriptStack::PhpScriptStack(size_t stack_size) noexcept
  : stack_size_((stack_size + getpagesize() - 1) / getpagesize() * getpagesize())
  , run_stack_(static_cast<char *>(std::aligned_alloc(getpagesize(), stack_size_)))
  , protected_end_(run_stack_ + getpagesize())
  , run_stack_end_(run_stack_ + stack_size_) {
  assert(mprotect(run_stack_, getpagesize(), PROT_NONE) == 0);
}

PhpScriptStack::~PhpScriptStack() noexcept {
  mprotect(run_stack_, getpagesize(), PROT_READ | PROT_WRITE);
  std::free(run_stack_);
}

bool PhpScriptStack::is_protected(const char *x) const noexcept {
  return run_stack_ <= x && x < protected_end_;
}

bool PhpScriptStack::check_stack_overflow(const char *x) const noexcept {
  assert(protected_end_ <= x && x < run_stack_end_);
  long left = x - protected_end_;
  return left < (1 << 12);
}

// asan_stack_unpoison marks the script stack memory as no longer in use,
// making it possible to do a longjmp up the stack;
// if ASAN is disabled, this function does nothing
// use this function right before doing a longjmp
void PhpScriptStack::asan_stack_unpoison() const noexcept {
#if ASAN_ENABLED
  ASAN_UNPOISON_MEMORY_REGION(run_stack_, stack_size_);
#endif
}

void PhpScriptStack::asan_stack_clear() const noexcept {
#if ASAN_ENABLED
  // give a chance for leak sanitizer to examine memory
  // (lsan gets SIGSEGV trying to access PROT_NONE memory)
  mprotect(run_stack_, getpagesize(), PROT_READ | PROT_WRITE);
  ASAN_UNPOISON_MEMORY_REGION(run_stack_, stack_size_);
  // clear stack of coroutine; this allows to treat all its content as non-live memory,
  // thus lsan will be unable to find pointer(locating on stack) to leaked object on heap,
  // and will report a leak
  std::memset(run_stack_, 0, stack_size_);
#endif
}

PhpScript::PhpScript(size_t mem_size, double oom_handling_memory_ratio, size_t stack_size) noexcept
  : mem_size(mem_size)
  , oom_handling_memory_ratio(oom_handling_memory_ratio)
  , run_mem(static_cast<char *>(mmap(nullptr, mem_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)))
  , script_stack(stack_size) {
  // fprintf (stderr, "PHPScriptBase: constructor\n");
  // fprintf (stderr, "[%p -> %p] [%p -> %p]\n", run_stack, run_stack_end, run_mem, run_mem + mem_size);
}

PhpScript::~PhpScript() noexcept {
  munmap(run_mem, mem_size);
}

void PhpScript::init(script_t *script, php_query_data *data_to_set) noexcept {
  assert (script != nullptr);
  assert_state(run_state_t::empty);

  query = nullptr;
  state = run_state_t::before_init;

  assert_state(run_state_t::before_init);

  getcontext_portable(&run_context);
  run_context.uc_stack.ss_sp = script_stack.get_stack_ptr();
  run_context.uc_stack.ss_size = script_stack.get_stack_size();
  run_context.uc_link = nullptr;
  makecontext_portable(&run_context, &script_context_entrypoint, 0);

  run_main = script;
  data = data_to_set;

  state = run_state_t::ready;

  error_message = "??? error";

  script_time = 0;
  net_time = 0;
  cur_timestamp = dl_time();
  queries_cnt = 0;
  long_queries_cnt = 0;

  query_stats_id++;
  memset(&query_stats, 0, sizeof(query_stats));

  PhpScript::memory_limit_exceeded = false;
}

int PhpScript::swapcontext_helper(ucontext_t_portable *oucp, const ucontext_t_portable *ucp) {
  stack_end = reinterpret_cast<char *>(ucp->uc_stack.ss_sp) + ucp->uc_stack.ss_size;
  return swapcontext_portable(oucp, ucp);
}

void PhpScript::pause() noexcept {
  try_run_shutdown_functions_on_timeout();
  //fprintf (stderr, "pause: \n");
  in_script_context = false;
#if ASAN_ENABLED
  __sanitizer_start_switch_fiber(nullptr, main_thread_stack, main_thread_stacksize);
#endif
  assert(swapcontext_helper(&run_context, &exit_context) == 0);
#if ASAN_ENABLED
  __sanitizer_finish_switch_fiber(nullptr, &main_thread_stack, &main_thread_stacksize);
#endif
  in_script_context = true;
  check_net_context_errors();
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_switch_context_to_script(last_net_time_delta);
  }
  //fprintf (stderr, "pause: ended\n");
}

void PhpScript::resume() noexcept {
#if ASAN_ENABLED
  __sanitizer_start_switch_fiber(nullptr, run_context.uc_stack.ss_sp, run_context.uc_stack.ss_size);
#endif
  assert(swapcontext_helper(&exit_context, &run_context) == 0);
#if ASAN_ENABLED
  __sanitizer_finish_switch_fiber(nullptr, nullptr, nullptr);
#endif
}

void dump_query_stats() {
  const size_t buf_size = 100;
  char buf[buf_size];
  char *s = buf;
  if (query_stats.desc != nullptr) {
    s += snprintf(s, buf_size, "%s:", query_stats.desc);
  }
  if (query_stats.port != 0) {
    s += snprintf(s, buf_size - (s - buf), "[port=%d]", query_stats.port);
  }
  if (query_stats.timeout > 0) {
    s += snprintf(s, buf_size - (s - buf), "[timeout=%lf]", query_stats.timeout);
  }
  *s = 0;
  kprintf("%s\n", buf);

  if (query_stats.query != nullptr) {
    const char *s = query_stats.query;
    const char *t = s;
    while (*t && *t != '\r' && *t != '\n') {
      t++;
    }
    if (t - s <= 1000) {
      kprintf("QUERY:[%.*s]\n", static_cast<int>(t - s), s);
    } else {
      kprintf("QUERY:[%.*s]<truncated, real length = %d>\n", 1000, s, static_cast<int>(t - s));
    }
  }
}

void PhpScript::update_net_time() noexcept {
  double new_cur_timestamp = dl_time();

  double net_add = new_cur_timestamp - cur_timestamp;
  if (net_add > 0.1) {
    if (query_stats.q_id == query_stats_id) {
      dump_query_stats();
    }
    ++long_queries_cnt;
    kprintf("LONG query: %lf\n", net_add);
    if (const net_event_t *event = get_last_net_event()) {
      kprintf("Awakening net event: %s\n", event->get_description());
    }
  }
  net_time += net_add;
  last_net_time_delta = net_add;

  cur_timestamp = new_cur_timestamp;
}

void PhpScript::update_script_time() noexcept {
  double new_cur_timestamp = dl_time();
  script_time += new_cur_timestamp - cur_timestamp;
  cur_timestamp = new_cur_timestamp;
}

run_state_t PhpScript::iterate() noexcept {
  current_script = this;

  assert_state(run_state_t::ready);
  state = run_state_t::running;

  update_net_time();
  notify_profiler_stop_waiting();

  resume();

  notify_profiler_start_waiting();
  update_script_time();

  queries_cnt += state == run_state_t::query;
  memset(&query_stats, 0, sizeof(query_stats));
  query_stats_id++;

  return state;
}

void PhpScript::finish() noexcept {
  assert (state == run_state_t::finished || state == run_state_t::error);
  assert(dl::is_malloc_replaced() == false);
  auto save_state = state;
  const auto &script_mem_stats = dl::get_script_memory_stats();
  state = run_state_t::uncleared;
  update_net_time();
  vk::singleton<ServerStats>::get().add_request_stats(script_time, net_time, queries_cnt, long_queries_cnt, script_mem_stats.max_memory_used,
                                                      script_mem_stats.max_real_memory_used, vk::singleton<CurlMemoryUsage>::get().total_allocated, error_type);
  if (save_state == run_state_t::error) {
    assert (error_message != nullptr);
    kprintf("Critical error during script execution: %s\n", error_message);
    kphp_tracing::on_php_script_finish_terminated();
  }
  if (save_state == run_state_t::error || script_mem_stats.real_memory_used >= 100000000) {
    if (data != nullptr) {
      http_query_data *http_data = data->http_data;
      if (http_data != nullptr) {
        assert (http_data->headers);

        kprintf("HEADERS: len = %d\n%.*s\nEND HEADERS\n", http_data->headers_len, min(http_data->headers_len, 1 << 16), http_data->headers);
        kprintf("POST: len = %d\n%.*s\nEND POST\n", http_data->post_len, min(http_data->post_len, 1 << 16), http_data->post == nullptr ? "" : http_data->post);
      }
    }
  }

  const size_t buf_size = 5000;
  static char buf[buf_size];
  buf[0] = 0;
  if (disable_access_log < 2) {
    if (data != nullptr) {
      http_query_data *http_data = data->http_data;
      if (http_data != nullptr) {
        if (disable_access_log) {
          snprintf(buf, buf_size, "[uri = %.*s?<truncated>]", min(http_data->uri_len, 200), http_data->uri);
        } else {
          snprintf(buf, buf_size, "[uri = %.*s?%.*s]", min(http_data->uri_len, 200), http_data->uri,
                  min(http_data->get_len, 4000), http_data->get);
        }
      }
    }
    kprintf("[worked = %.3lf, net = %.3lf, script = %.3lf, queries_cnt = %5d, long_queries_cnt = %5d, static_memory = %9d, peak_memory = %9d, total_memory = %9d] %s\n",
            script_time + net_time, net_time, script_time, queries_cnt, long_queries_cnt,
            (int)dl::get_heap_memory_used(),
            (int)script_mem_stats.max_real_memory_used,
            (int)script_mem_stats.real_memory_used, buf);
  }
}

void PhpScript::clear() noexcept {
  assert_state(run_state_t::uncleared);
  run_main->clear();
  free_runtime_environment();
  state = run_state_t::empty;
  if (use_madvise_dontneed) {
    if (dl::get_script_memory_stats().real_memory_used > memory_used_to_recreate_script) {
      const int advice = madvise_madv_free_supported() ? MADV_FREE : MADV_DONTNEED;
      our_madvise(&run_mem[memory_used_to_recreate_script], mem_size - memory_used_to_recreate_script, advice);
    }
  }
  script_stack.asan_stack_clear();
}

void PhpScript::assert_state(run_state_t expected) {
  if (state != expected) {
    log_server_critical("assert_state failed: state is %d, expected %d", static_cast<int>(state), static_cast<int>(expected));
  }
  assert(state == expected);
}

void PhpScript::ask_query(php_query_base_t *q) noexcept {
  assert_state(run_state_t::running);
  query = q;
  state = run_state_t::query;
  //fprintf (stderr, "ask_query: pause\n");
  pause();
  //fprintf (stderr, "ask_query: after pause\n");
}

void PhpScript::set_script_result(script_result *res_to_set) noexcept {
  assert_state(run_state_t::running);
  res = res_to_set;
  state = run_state_t::finished;
  pause();
}

void PhpScript::query_readed() noexcept {
  assert (in_script_context == false);
  assert_state(run_state_t::query);
  state = run_state_t::query_running;
}

void PhpScript::query_answered() noexcept {
  assert_state(run_state_t::query_running);
  state = run_state_t::ready;
  //fprintf (stderr, "ok\n");
}

void PhpScript::run() noexcept {
  if (data != nullptr) {
    http_query_data *http_data = data->http_data;
    if (http_data != nullptr) {
      //fprintf (stderr, "arguments\n");
      //fprintf (stderr, "[uri = %.*s]\n", http_data->uri_len, http_data->uri);
      //fprintf (stderr, "[get = %.*s]\n", http_data->get_len, http_data->get);
      //fprintf (stderr, "[headers = %.*s]\n", http_data->headers_len, http_data->headers);
      //fprintf (stderr, "[post = %.*s]\n", http_data->post_len, http_data->post);
    }

    rpc_query_data *rpc_data = data->rpc_data;
    if (rpc_data != nullptr) {
      /*
      fprintf (stderr, "N = %d\n", rpc_data->len);
      for (int i = 0; i < rpc_data->len; i++) {
        fprintf (stderr, "%d: %10d\n", i, rpc_data->data[i]);
      }
      */
    }
  }
  assert (run_main->run != nullptr);

  dl::enter_critical_section();
  in_script_context = true;
  auto oom_handling_memory_size = static_cast<size_t>(std::ceil(mem_size * oom_handling_memory_ratio));
  auto script_memory_size = mem_size - oom_handling_memory_size;
  init_runtime_environment(data, run_mem, script_memory_size, oom_handling_memory_size);
  dl::leave_critical_section();
  php_assert (dl::in_critical_section == 0); // To ensure that no critical section is left at the end of the initialization
  check_net_context_errors();

  CurException = Optional<bool>{};
  run_main->run();
  if (CurException.is_null()) {
    set_script_result(nullptr);
  } else {
    // we can reach this point during normal script execution or during the shutdown functions execution
    // (they can start normally or from a timeout context);
    // execute the shutdown functions only if we're here outside the shutdown functions context
    // also note that we're running shutdown functions normally (like from the exit) and that will rewind the timeout timer to 0
    //
    // why not calling shutdown functions from error() functions itself?
    // most errors are not recoverable and may happen from signal handler
    // PHP can't run shutdown functions in case of the stack overflow or when heap allocations limit is exceeded,
    // therefore we shouldn't bother either;
    // we handle exception script error here as it's clearly the script context while error() could be called
    // from a signal handler
    if (get_shutdown_functions_count() != 0 && get_shutdown_functions_status() == shutdown_functions_status::not_executed) {
      // run shutdown functions with an empty exception context; then recover it before we proceed
      auto saved_exception = CurException;
      CurException = Optional<bool>{};
      run_shutdown_functions_from_script(ShutdownType::exception);
      // only nothrow shutdown callbacks are allowed by the compiler, so the CurException should be null
      php_assert(CurException.is_null());
      CurException = saved_exception;
    }
    error(php_uncaught_exception_error(CurException), script_error_t::exception);
  }
}

void PhpScript::reset_script_timeout() noexcept {
  // php_script_set_timeout has a side effect of setting the PhpScript::time_limit_exceeded to false;
  // and we really do need this before executing shutdown functions otherwise shutdown functions will be terminated
  // after the first swap context back to script at PhpScript::check_net_context_errors()
  set_timeout(script_timeout);
}

double PhpScript::get_net_time() const noexcept {
  return net_time;
}

long long PhpScript::memory_get_total_usage() const noexcept {
  return dl::get_script_memory_stats().real_memory_used;
}

double PhpScript::get_script_time() noexcept {
  assert_state(run_state_t::running);
  update_script_time();
  return script_time;
}

int PhpScript::get_net_queries_count() const noexcept {
  return queries_cnt;
}

PhpScript *volatile PhpScript::current_script;
ucontext_t_portable PhpScript::exit_context;
volatile bool PhpScript::in_script_context = false;
volatile bool PhpScript::time_limit_exceeded = false;
volatile bool PhpScript::memory_limit_exceeded = false;

static __inline__ void *get_sp() {
  return __builtin_frame_address(0);
}

void check_stack_overflow() __attribute__ ((noinline));

void check_stack_overflow() {
  if (PhpScript::in_script_context) {
    void *sp = get_sp();
    if (PhpScript::current_script->script_stack.check_stack_overflow(static_cast<char *>(sp))) {
      vk::singleton<JsonLogger>::get().write_stack_overflow_log(E_ERROR);
      raise(SIGSTACKOVERFLOW);
      fprintf(stderr, "_exiting in check_stack_overflow\n");
      _exit(1);
    }
  }
}

void PhpScript::disable_timeout() noexcept {
  itimerval timer{.it_interval{0, 0}, .it_value{0, 0}};
  setitimer(ITIMER_REAL, &timer, nullptr);
}

void PhpScript::set_timeout(double t) noexcept {
  disable_timeout();
  static itimerval timer;

  if (t > MAX_SCRIPT_TIMEOUT) {
    return;
  }

  int sec = (int)t, usec = (int)((t - sec) * 1000000);
  timer.it_value.tv_sec = sec;
  timer.it_value.tv_usec = usec;

  PhpScript::time_limit_exceeded = false;
  setitimer(ITIMER_REAL, &timer, nullptr);
}

void PhpScript::script_context_entrypoint() noexcept {
#if ASAN_ENABLED
  __sanitizer_finish_switch_fiber(nullptr, &main_thread_stack, &main_thread_stacksize);
#endif
  current_script->run();
}

void PhpScript::terminate(const char *error_message_, script_error_t error_type_) noexcept {
  state = run_state_t::error;
  error_type = error_type_;
  error_message = error_message_;
}
