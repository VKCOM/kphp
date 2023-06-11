// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/signal-handlers.h"

#include <execinfo.h>
#include <sys/time.h>

#include "common/kprintf.h"
#include "common/server/crash-dump.h"
#include "common/server/signals.h"
#include "runtime/critical_section.h"
#include "runtime/interface.h"
#include "server/json-logger.h"
#include "server/php-engine-vars.h"
#include "server/server-log.h"

namespace {

void kwrite_str(int fd, const char *s) noexcept {
  kwrite(fd, s, static_cast<int>(strlen(s)));
}

void write_str(int fd, const char *s) noexcept {
  write(fd, s, std::min(strlen(s), size_t{1000}));
}

bool check_signal_critical_section(int sig_num, const char *sig_name) {
  if (dl::in_critical_section) {
    const size_t message_1kw_size = 100;
    char message_1kw[message_1kw_size];
    snprintf(message_1kw, message_1kw_size, "in critical section: pending %s caught\n", sig_name);
    kwrite_str(2, message_1kw);
    dl::pending_signals = dl::pending_signals | (1ll << sig_num);
    return false;
  }
  dl::pending_signals = 0;
  return true;
}

void default_sigalrm_handler(int signum) {
  // Always used in job workers.
  // It's critical for job workers to terminate as soon as possible after normal timeout.
  // Because timeouts are usually small and always used for fine-grained job duration control
  kwrite_str(2, "in default_sigalrm_handler\n");
  if (check_signal_critical_section(signum, "SIGALRM")) {
    PhpScript::time_limit_exceeded = true;
    if (PhpScript::in_script_context) {
      if (is_json_log_on_timeout_enabled) {
        vk::singleton<JsonLogger>::get().write_log_with_backtrace("Maximum execution time exceeded", E_ERROR);
      }
      perform_error_if_running("timeout exit\n", script_error_t::timeout);
    }
  }
}

void sigalrm_handler(int signum) {
  kwrite_str(2, "in sigalrm_handler\n");
  if (check_signal_critical_section(signum, "SIGALRM")) {
    // There are 3 possible situations when a timeout occurs
    if (!PhpScript::in_script_context) {
      // [1] code in net context
      // save the timeout fact in order to process it in the script context
      PhpScript::time_limit_exceeded = true;
    } else if (!PhpScript::time_limit_exceeded) {
      // [2] code in script context and this is the first timeout
      // process timeout
      PhpScript::time_limit_exceeded = true;
      if (is_json_log_on_timeout_enabled) {
        vk::singleton<JsonLogger>::get().write_log_with_backtrace("Maximum execution time exceeded", E_ERROR);
      }
      if (get_shutdown_functions_count() > 0) {
        // setup hard timeout which is deadline of shutdown functions call @see try_run_shutdown_functions_on_timeout
        static itimerval timer;
        memset(&timer, 0, sizeof(itimerval));
        timer.it_value.tv_sec = static_cast<decltype(timer.it_value.tv_sec)>(std::floor(hard_timeout));
        timer.it_value.tv_usec = static_cast<decltype(timer.it_value.tv_usec)>(1e6 * (hard_timeout - std::floor(hard_timeout)));
        setitimer(ITIMER_REAL, &timer, nullptr);
      } else {
        // if there's no shutdown functions terminate script now
        perform_error_if_running("soft timeout exit\n", script_error_t::timeout);
      }
    } else {
      kwrite_str(2, "hard timeout expired\n");
      // [3] code in script context and this is the second timeout
      // time to start shutdown functions has expired, emergency shutdown
      perform_error_if_running("hard timeout exit\n", script_error_t::timeout);
    }
  }
}

void sigusr2_handler(int signum) {
  kwrite_str(2, "in sigusr2_handler\n");
  if (check_signal_critical_section(signum, "SIGUSR2")) {
    PhpScript::memory_limit_exceeded = true;
    if (PhpScript::in_script_context) {
      perform_error_if_running("memory limit exit\n", script_error_t::memory_limit);
    }
  }
}

void php_assert_handler(int signum) {
  kwrite_str(2, "in php_assert_handler (SIGRTMIN+1 signal)\n");
  if (check_signal_critical_section(signum, "SIGRTMIN+1")) {
    perform_error_if_running("php assert error\n", script_error_t::php_assert);
  }
}

void stack_overflow_handler(int signum) {
  kwrite_str(2, "in stack_overflow_handler (SIGRTMIN+2 signal)\n");
  if (check_signal_critical_section(signum, "SIGRTMIN+2")) {
    perform_error_if_running("stack overflow error\n", script_error_t::stack_overflow);
  }
}

void print_http_data() {
  if (!PhpScript::in_script_context) {
    return;
  }
  if (!PhpScript::current_script) {
    write_str(2, "\nPHPScriptBase::current_script is nullptr\n");
  } else if (PhpScript::current_script->data) {
    if (http_query_data *data = PhpScript::current_script->data->http_data) {
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
  if (PhpScript::in_script_context && PhpScript::current_script->script_stack.is_protected(static_cast<char *>(addr))) {
    vk::singleton<JsonLogger>::get().write_stack_overflow_log(E_ERROR);
    write_str(2, "Error -1: Callstack overflow\n");
    print_http_data();
    dl_print_backtrace(trace, trace_size);
    if (dl::in_critical_section) {
      vk::singleton<JsonLogger>::get().fsync_log_file();
      kwrite_str(2, "In critical section: calling _exit (124)\n");
      _exit(124);
    } else {
      PhpScript::error("sigsegv(stack overflow)", script_error_t::stack_overflow);
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
} // namespace


//mark no return
void perform_error_if_running(const char *msg, script_error_t error_type) {
  if (PhpScript::in_script_context) {
    kwrite_str(2, msg);
    PhpScript::error(msg, error_type);
    assert ("unreachable point" && 0);
  }
}

//C interface
void init_handlers() {
  constexpr size_t SEGV_STACK_SIZE = 65536;
  static std::array<char, SEGV_STACK_SIZE> buffer;

  stack_t segv_stack;
  segv_stack.ss_sp = buffer.data();
  segv_stack.ss_flags = 0;
  segv_stack.ss_size = SEGV_STACK_SIZE;
  sigaltstack(&segv_stack, nullptr);

  ksignal(SIGALRM, default_sigalrm_handler);
  ksignal(SIGUSR2, sigusr2_handler);
  ksignal(SIGPHPASSERT, php_assert_handler);
  ksignal(SIGSTACKOVERFLOW, stack_overflow_handler);

  dl_sigaction(SIGSEGV, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigsegv_handler);
  dl_sigaction(SIGBUS, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigsegv_handler);
  dl_signal(SIGABRT, sigabrt_handler);
}

void worker_global_init_handlers(WorkerType worker_type) {
  if (worker_type == WorkerType::general_worker && hard_timeout > 0) {
    ksignal(SIGALRM, sigalrm_handler);
  }
}
