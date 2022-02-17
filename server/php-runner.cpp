// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-runner.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <execinfo.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "common/fast-backtrace.h"
#include "common/kernel-version.h"
#include "common/kprintf.h"
#include "common/server/crash-dump.h"
#include "common/server/signals.h"
#include "common/wrappers/memory-utils.h"
#include "net/net-connections.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/curl.h"
#include "runtime/exception.h"
#include "runtime/interface.h"
#include "runtime/profiler.h"
#include "server/json-logger.h"
#include "server/php-engine-vars.h"
#include "server/php-queries.h"
#include "server/server-log.h"
#include "server/server-stats.h"

query_stats_t query_stats;
long long query_stats_id = 1;

void PHPScriptBase::error(const char *error_message, script_error_t error_type) {
  assert (is_running == true);
  is_running = false;
  if (dl::is_malloc_replaced()) {
    // in case the error happened when malloc was replaced
    dl::rollback_malloc_replacement();
  }
  current_script->state = run_state_t::error;
  current_script->error_message = error_message;
  current_script->error_type = error_type;
  stack_end = reinterpret_cast<char *>(exit_context.uc_stack.ss_sp) + exit_context.uc_stack.ss_size;
#if ASAN7_ENABLED
  __sanitizer_finish_switch_fiber(nullptr, nullptr, nullptr);
  __sanitizer_start_switch_fiber(nullptr, exit_context.uc_stack.ss_sp, exit_context.uc_stack.ss_size);
#endif
  setcontext_portable(&exit_context);
}

void PHPScriptBase::check_tl() {
  if (tl_flag) {
    state = run_state_t::error;
    error_type = script_error_t::timeout;
    error_message = "Timeout exit";
    pause();
  }
}

bool PHPScriptBase::is_protected(char *x) {
  return run_stack <= x && x < protected_end;
}

bool PHPScriptBase::check_stack_overflow(char *x) {
  assert (protected_end <= x && x < run_stack_end);
  long left = x - protected_end;
  return left < (1 << 12);
}


PHPScriptBase::PHPScriptBase(size_t mem_size, size_t stack_size) :
  cur_timestamp(0),
  net_time(0),
  script_time(0),
  queries_cnt(0),
  long_queries_cnt(0),
  state(run_state_t::empty),
  error_message(nullptr),
  error_type(script_error_t::no_error),
  query(nullptr),
  run_stack(nullptr),
  protected_end(nullptr),
  run_stack_end(nullptr),
  run_mem(nullptr),
  mem_size(mem_size),
  stack_size(stack_size),
  run_context(),
  run_main(nullptr),
  data(nullptr),
  res(nullptr) {
  //fprintf (stderr, "PHPScriptBase: constructor\n");
  stack_size = getpagesize() + (stack_size + getpagesize() - 1) / getpagesize() * getpagesize();
  run_stack = (char *)valloc(stack_size);
  assert (mprotect(run_stack, getpagesize(), PROT_NONE) == 0);
  protected_end = run_stack + getpagesize();
  run_stack_end = run_stack + stack_size;

  run_mem = static_cast<char *>(mmap(nullptr, mem_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
  //fprintf (stderr, "[%p -> %p] [%p -> %p]\n", run_stack, run_stack_end, run_mem, run_mem + mem_size);
}

PHPScriptBase::~PHPScriptBase() {
#if ASAN7_ENABLED
  if (fiber_is_started) {
    __sanitizer_finish_switch_fiber(nullptr, nullptr, nullptr);
  }
#endif
  mprotect(run_stack, getpagesize(), PROT_READ | PROT_WRITE);
  free(run_stack);
  munmap(run_mem, mem_size);
}

void PHPScriptBase::init(script_t *script, php_query_data *data_to_set) {
  assert (script != nullptr);
  assert (state == run_state_t::empty);

  query = nullptr;
  state = run_state_t::before_init;

  assert (state == run_state_t::before_init);

  getcontext_portable(&run_context);
  run_context.uc_stack.ss_sp = run_stack;
  run_context.uc_stack.ss_size = stack_size;
  run_context.uc_link = nullptr;
  makecontext_portable(&run_context, &cur_run, 0);

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

  PHPScriptBase::ml_flag = false;
}

int PHPScriptBase::swapcontext_helper(ucontext_t_portable *oucp, const ucontext_t_portable *ucp) {
  stack_end = reinterpret_cast<char *>(ucp->uc_stack.ss_sp) + ucp->uc_stack.ss_size;
#if ASAN7_ENABLED
  if (fiber_is_started) {
    __sanitizer_finish_switch_fiber(nullptr, nullptr, nullptr);
  }
  fiber_is_started = true;
  __sanitizer_start_switch_fiber(nullptr, ucp->uc_stack.ss_sp, ucp->uc_stack.ss_size);
#endif

  return swapcontext_portable(oucp, ucp);
}
void PHPScriptBase::pause() {
  //fprintf (stderr, "pause: \n");
  is_running = false;
  assert (swapcontext_helper(&run_context, &exit_context) == 0);
  is_running = true;
  check_tl();
  //fprintf (stderr, "pause: ended\n");
}

void PHPScriptBase::resume() {
  assert (swapcontext_helper(&exit_context, &run_context) == 0);
}

void dump_query_stats() {
  char tmp[100];
  char *s = tmp;
  if (query_stats.desc != nullptr) {
    s += sprintf(s, "%s:", query_stats.desc);
  }
  if (query_stats.port != 0) {
    s += sprintf(s, "[port=%d]", query_stats.port);
  }
  if (query_stats.timeout > 0) {
    s += sprintf(s, "[timeout=%lf]", query_stats.timeout);
  }
  *s = 0;
  kprintf("%s\n", tmp);

  if (query_stats.query != nullptr) {
    const char *s = query_stats.query;
    const char *t = s;
    while (*t && *t != '\r' && *t != '\n') {
      t++;
    }
    if (t - s <= 1000) {
      kprintf("QUERY:[%.*s]\n", (int)(t - s), s);
    } else {
      kprintf("QUERY:[%.*s]<truncated, real length = %d>\n", 1000, s, (int)(t - s));
    }
  }
}

void PHPScriptBase::update_net_time() {
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

  cur_timestamp = new_cur_timestamp;
}

void PHPScriptBase::update_script_time() {
  double new_cur_timestamp = dl_time();
  script_time += new_cur_timestamp - cur_timestamp;
  cur_timestamp = new_cur_timestamp;
}

run_state_t PHPScriptBase::iterate() {
  current_script = this;

  assert (state == run_state_t::ready);
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

void PHPScriptBase::finish() {
  assert (state == run_state_t::finished || state == run_state_t::error);
  auto save_state = state;
  const auto &script_mem_stats = dl::get_script_memory_stats();
  state = run_state_t::uncleared;
  update_net_time();
  vk::singleton<ServerStats>::get().add_request_stats(script_time, net_time, queries_cnt, long_queries_cnt, script_mem_stats.max_memory_used,
                                                      script_mem_stats.max_real_memory_used, vk::singleton<CurlMemoryUsage>::get().total_allocated, error_type);
  if (save_state == run_state_t::error) {
    assert (error_message != nullptr);
    kprintf("Critical error during script execution: %s\n", error_message);
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

  static char buf[5000];
  buf[0] = 0;
  if (disable_access_log < 2) {
    if (data != nullptr) {
      http_query_data *http_data = data->http_data;
      if (http_data != nullptr) {
        if (disable_access_log) {
          sprintf(buf, "[uri = %.*s?<truncated>]", min(http_data->uri_len, 200), http_data->uri);
        } else {
          sprintf(buf, "[uri = %.*s?%.*s]", min(http_data->uri_len, 200), http_data->uri,
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

void PHPScriptBase::clear() {
  assert(state == run_state_t::uncleared);
  run_main->clear();
  free_runtime_environment();
  state = run_state_t::empty;
  if (use_madvise_dontneed) {
    if (dl::get_script_memory_stats().real_memory_used > memory_used_to_recreate_script) {
      const int advice = madvise_madv_free_supported() ? MADV_FREE : MADV_DONTNEED;
      our_madvise(&run_mem[memory_used_to_recreate_script], mem_size - memory_used_to_recreate_script, advice);
    }
  }
}

void PHPScriptBase::ask_query(php_query_base_t *q) {
  assert (state == run_state_t::running);
  query = q;
  state = run_state_t::query;
  //fprintf (stderr, "ask_query: pause\n");
  pause();
  //fprintf (stderr, "ask_query: after pause\n");
}

void PHPScriptBase::set_script_result(script_result *res_to_set) {
  assert (state == run_state_t::running);
  res = res_to_set;
  state = run_state_t::finished;
  pause();
}

void PHPScriptBase::query_readed() {
  assert (is_running == false);
  assert (state == run_state_t::query);
  state = run_state_t::query_running;
}

void PHPScriptBase::query_answered() {
  assert (state == run_state_t::query_running);
  state = run_state_t::ready;
  //fprintf (stderr, "ok\n");
}

void PHPScriptBase::run() {
  is_running = true;
  check_tl();

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

  init_runtime_environment(data, run_mem, mem_size);
  CurException = Optional<bool>{};
  run_main->run();
  if (CurException.is_null()) {
    set_script_result(nullptr);
  } else {
    error(php_uncaught_exception_error(CurException), script_error_t::exception);
  }
}

double PHPScriptBase::get_net_time() const {
  return net_time;
}

long long PHPScriptBase::memory_get_total_usage() const {
  return dl::get_script_memory_stats().real_memory_used;
}

double PHPScriptBase::get_script_time() {
  assert (state == run_state_t::running);
  update_script_time();
  return script_time;
}

int PHPScriptBase::get_net_queries_count() const {
  return queries_cnt;
}


PHPScriptBase *volatile PHPScriptBase::current_script;
ucontext_t_portable PHPScriptBase::exit_context;
volatile bool PHPScriptBase::is_running = false;
volatile bool PHPScriptBase::tl_flag = false;
volatile bool PHPScriptBase::ml_flag = false;

//TODO: sometimes I need to call old handlers
//TODO: recheck!
void kwrite_str(int fd, const char *s) noexcept {
  kwrite(fd, s, static_cast<int>(strlen(s)));
}

void write_str(int fd, const char *s) noexcept {
  write(fd, s, std::min(strlen(s), size_t{1000}));
}

namespace kphp_runtime_signal_handlers {
namespace {
bool check_signal_critical_section(int sig_num, const char *sig_name) {
  if (dl::in_critical_section) {
    char message_1kw[100];
    sprintf(message_1kw, "in critical section: pending %s caught\n", sig_name);
    kwrite_str(2, message_1kw);
    dl::pending_signals = dl::pending_signals | (1 << sig_num);
    return false;
  }
  dl::pending_signals = 0;
  return true;
}

void perform_error_if_running(const char *msg, script_error_t error_type) {
  if (PHPScriptBase::is_running) {
    kwrite_str(2, msg);
    PHPScriptBase::error(msg, error_type);
    assert ("unreachable point" && 0);
  }
}
} // namespace

static void sigalrm_handler(int signum) {
  kwrite_str(2, "in sigalrm_handler\n");
  if (check_signal_critical_section(signum, "SIGALRM")) {
    PHPScriptBase::tl_flag = true;
    if (PHPScriptBase::is_running && is_json_log_on_timeout_enabled) {
      vk::singleton<JsonLogger>::get().write_log_with_backtrace("Maximum execution time exceeded", E_ERROR);
    }
    perform_error_if_running("timeout exit\n", script_error_t::timeout);
  }
}

static void sigusr2_handler(int signum) {
  kwrite_str(2, "in sigusr2_handler\n");
  if (check_signal_critical_section(signum, "SIGUSR2")) {
    PHPScriptBase::ml_flag = true;
    perform_error_if_running("memory limit exit\n", script_error_t::memory_limit);
  }
}

static void php_assert_handler(int signum) {
  kwrite_str(2, "in php_assert_handler (SIGRTMIN+1 signal)\n");
  if (check_signal_critical_section(signum, "SIGRTMIN+1")) {
    perform_error_if_running("php assert error\n", script_error_t::php_assert);
  }
}

static void stack_overflow_handler(int signum) {
  kwrite_str(2, "in stack_overflow_handler (SIGRTMIN+2 signal)\n");
  if (check_signal_critical_section(signum, "SIGRTMIN+2")) {
    perform_error_if_running("stack overflow error\n", script_error_t::stack_overflow);
  }
}
}

void print_http_data() {
  if (!PHPScriptBase::is_running) {
    return;
  }
  if (!PHPScriptBase::current_script) {
    write_str(2, "\nPHPScriptBase::current_script is nullptr\n");
  } else if (PHPScriptBase::current_script->data) {
    if (http_query_data *data = PHPScriptBase::current_script->data->http_data) {
      write_str(2, "\nuri\n");
      write(2, data->uri, data->uri_len);
      write_str(2, "\nget\n");
      write(2, data->get, data->get_len);
      write_str(2, "\nheaders\n");
      write(2, data->headers, data->headers_len);
      write_str(2, "\npost\n");
      if (data->post && data->post_len > 0) {
        write(2, data->post, data->post_len);
      }
    }
  }
}

void print_prologue(int64_t cur_time) noexcept {
  write_str(2, engine_tag);

  char buf[13];
  char *s = buf + 13;
  auto t = static_cast<int>(cur_time);
  *--s = 0;
  do {
    *--s = static_cast<char>(t % 10 + '0');
    t /= 10;
  } while (t > 0);
  write_str(2, s);
  write_str(2, engine_pid);
}

void kill_workers() noexcept {
#if defined(__APPLE__)
  if (master_flag == 1) {
    killpg(getpgid(pid), SIGKILL);
  }
#endif
}

void sigsegv_handler(int signum, siginfo_t *info, void *ucontext) {
  crash_dump_write(static_cast<ucontext_t *>(ucontext));

  const int64_t cur_time = time(nullptr);
  print_prologue(cur_time);

  void *trace[64];
  const int trace_size = backtrace(trace, 64);

  void *addr = info->si_addr;
  if (PHPScriptBase::is_running && PHPScriptBase::current_script->is_protected(static_cast<char *>(addr))) {
    vk::singleton<JsonLogger>::get().write_stack_overflow_log(E_ERROR);
    write_str(2, "Error -1: Callstack overflow\n");
    print_http_data();
    dl_print_backtrace(trace, trace_size);
    if (dl::in_critical_section) {
      vk::singleton<JsonLogger>::get().fsync_log_file();
      kwrite_str(2, "In critical section: calling _exit (124)\n");
      _exit(124);
    } else {
      PHPScriptBase::error("sigsegv(stack overflow)", script_error_t::stack_overflow);
    }
  } else {
    const char *msg = signum == SIGBUS ? "SIGBUS terminating program" : "SIGSEGV terminating program";
    vk::singleton<JsonLogger>::get().write_log(msg, static_cast<int>(ServerLog::Critical), cur_time, trace, trace_size, true);
    vk::singleton<JsonLogger>::get().fsync_log_file();
    write_str(2, "Error -2: Segmentation fault\n");
    print_http_data();
    dl_print_backtrace(trace, trace_size);
    kill_workers();
    raise(SIGQUIT); // hack for generate core dump
    _exit(123);
  }
}

void sigabrt_handler(int) {
  const int64_t cur_time = time(nullptr);
  void *trace[64];
  const int trace_size = backtrace(trace, 64);
  vk::string_view msg{dl_get_assert_message()};
  if (msg.empty()) {
    msg = "SIGABRT terminating program";
  }
  vk::singleton<JsonLogger>::get().write_log(msg, static_cast<int>(ServerLog::Critical), cur_time, trace, trace_size, true);
  vk::singleton<JsonLogger>::get().fsync_log_file();

  print_prologue(cur_time);
  write_str(2, "SIGABRT terminating program\n");
  print_http_data();
  dl_print_backtrace(trace, trace_size);
  kill_workers();
  _exit(EXIT_FAILURE);
}

static __inline__ void *get_sp() {
  return __builtin_frame_address(0);
}

void check_stack_overflow() __attribute__ ((noinline));

void check_stack_overflow() {
  if (PHPScriptBase::is_running) {
    void *sp = get_sp();
    if (PHPScriptBase::current_script->check_stack_overflow(static_cast<char *>(sp))) {
      vk::singleton<JsonLogger>::get().write_stack_overflow_log(E_ERROR);
      raise(SIGSTACKOVERFLOW);
      fprintf(stderr, "_exiting in check_stack_overflow\n");
      _exit(1);
    }
  }
}

//C interface
void init_handlers() {
  stack_t segv_stack;
  segv_stack.ss_sp = valloc(SEGV_STACK_SIZE);
  segv_stack.ss_flags = 0;
  segv_stack.ss_size = SEGV_STACK_SIZE;
  sigaltstack(&segv_stack, nullptr);

  ksignal(SIGALRM, kphp_runtime_signal_handlers::sigalrm_handler);
  ksignal(SIGUSR2, kphp_runtime_signal_handlers::sigusr2_handler);
  ksignal(SIGPHPASSERT, kphp_runtime_signal_handlers::php_assert_handler);
  ksignal(SIGSTACKOVERFLOW, kphp_runtime_signal_handlers::stack_overflow_handler);

  dl_sigaction(SIGSEGV, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigsegv_handler);
  dl_sigaction(SIGBUS, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigsegv_handler);
  dl_signal(SIGABRT, sigabrt_handler);
}

void php_script_finish(void *ptr) {
  ((PHPScriptBase *)ptr)->finish();
}

void php_script_clear(void *ptr) {
  ((PHPScriptBase *)ptr)->clear();
}

void php_script_free(void *ptr) {
  delete (PHPScriptBase *)ptr;
}

void php_script_init(void *ptr, script_t *f_run, php_query_data *data_to_set) {
  ((PHPScriptBase *)ptr)->init(f_run, data_to_set);
}

run_state_t php_script_iterate(void *ptr) {
  return ((PHPScriptBase *)ptr)->iterate();
}

php_query_base_t *php_script_get_query(void *ptr) {
  return ((PHPScriptBase *)ptr)->query;
}

script_result *php_script_get_res(void *ptr) {
  return ((PHPScriptBase *)ptr)->res;
}

void php_script_query_readed(void *ptr) {
  ((PHPScriptBase *)ptr)->query_readed();
}

void php_script_query_answered(void *ptr) {
  ((PHPScriptBase *)ptr)->query_answered();
}

run_state_t php_script_get_state(void *ptr) {
  return ((PHPScriptBase *)ptr)->state;
}

void *php_script_create(size_t mem_size, size_t stack_size) {
  return (void *)(new PHPScriptBase(mem_size, stack_size));
}

void php_script_terminate(void *ptr, const char *error_message, script_error_t error_type) {
  ((PHPScriptBase *)ptr)->state = run_state_t::error;
  ((PHPScriptBase *)ptr)->error_type = error_type;
  ((PHPScriptBase *)ptr)->error_message = error_message;
}

const char *php_script_get_error(void *ptr) {
  return static_cast<PHPScriptBase *>(ptr)->error_message;
}

script_error_t php_script_get_error_type(void *ptr) {
  return static_cast<PHPScriptBase *>(ptr)->error_type;
}

long long php_script_memory_get_total_usage(void *ptr) {
  return ((PHPScriptBase *)ptr)->memory_get_total_usage();
}

void php_script_set_timeout(double t) {
  static itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;

  //stop old timer
  setitimer(ITIMER_REAL, &timer, nullptr);

  if (t > MAX_SCRIPT_TIMEOUT) {
    return;
  }

  int sec = (int)t, usec = (int)((t - sec) * 1000000);
  timer.it_value.tv_sec = sec;
  timer.it_value.tv_usec = usec;

  PHPScriptBase::tl_flag = false;
  setitimer(ITIMER_REAL, &timer, nullptr);
}

void PHPScriptBase::cur_run() {
  current_script->run();
}
