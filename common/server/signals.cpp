// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/signals.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <unordered_set>

#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/options.h"
#include "common/server/crash-dump.h"
#include "common/server/engine-settings.h"
#include "common/version-string.h"

// TODO: it's strange file for it
int daemonize;

OPTION_PARSER_SHORT(OPT_GENERIC, "daemonize", 'd', optional_argument, "changes between daemonize/not daemonize mode") {
  if (!optarg) {
    daemonize ^= 1;
  } else {
    daemonize = atoi(optarg) != 0;
  }
  return 0;
}

static void write_to_stderr(const char *str, bool first = false) {
  if (first) {
    kwrite(STDERR_FILENO, str, strlen(str));
  } else {
    auto written = write(STDERR_FILENO, str, strlen(str));
    static_cast<void>(written);
  }
}

extra_debug_handler_t extra_debug_handler;
static volatile int double_print_backtrace_guard;

void print_backtrace() {
  if (double_print_backtrace_guard) {
    kwrite(STDERR_FILENO, "\n---Ignoring recursive print backtrace---\n", 42);
    return;
  }
  double_print_backtrace_guard = 1;
  void *buffer[64];
  int nptrs = backtrace(buffer, 64);
  kwrite(STDERR_FILENO, "\n------- Stack Backtrace -------\n", 33);
  backtrace_symbols_fd(buffer, nptrs, 2);
  kwrite(STDERR_FILENO, "-------------------------------\n", 32);
  const char *version_string = get_version_string();
  int l = strlen(version_string);
  kwrite(STDERR_FILENO, version_string, l);
  kwrite(STDERR_FILENO, "\n", 1);
  double_print_backtrace_guard = 0;
  if (extra_debug_handler) {
    extra_debug_handler_t debug_handler = extra_debug_handler;
    extra_debug_handler = nullptr;
    debug_handler();
  }
}

static pthread_t debug_main_pthread_id;

void kill_main() {
  if (debug_main_pthread_id && debug_main_pthread_id != pthread_self()) {
    pthread_kill(debug_main_pthread_id, SIGABRT);
  }
}

static void dump_stats() {
  const char *buf = nullptr;
  if (engine_settings_handlers.char_stats) {
    buf = engine_settings_handlers.char_stats();
  }
  if (buf) {
    write_to_stderr(buf, true);
  }
}

const char *signal_shortname(int sig) {
  switch (sig) {
    case SIGHUP:
      return "SIGHUP";
    case SIGINT:
      return "SIGINT";
    case SIGQUIT:
      return "SIGQUIT";
    case SIGILL:
      return "SIGILL";
    case SIGTRAP:
      return "SIGTRAP";
    case SIGABRT:
      return "SIGABRT";
    case SIGBUS:
      return "SIGBUS";
    case SIGFPE:
      return "SIGFPE";
    case SIGKILL:
      return "SIGKILL";
    case SIGUSR1:
      return "SIGUSR1";
    case SIGSEGV:
      return "SIGSEGV";
    case SIGUSR2:
      return "SIGUSR2";
    case SIGPIPE:
      return "SIGPIPE";
    case SIGALRM:
      return "SIGALRM";
    case SIGTERM:
      return "SIGTERM";
    case SIGCHLD:
      return "SIGCHLD";
    case SIGCONT:
      return "SIGCONT";
    case SIGSTOP:
      return "SIGSTOP";
    case SIGTSTP:
      return "SIGTSTP";
    case SIGTTIN:
      return "SIGTTIN";
    case SIGTTOU:
      return "SIGTTOU";
    case SIGURG:
      return "SIGURG";
    case SIGXCPU:
      return "SIGXCPU";
    case SIGXFSZ:
      return "SIGXFSZ";
    case SIGVTALRM:
      return "SIGVTALRM";
    case SIGPROF:
      return "SIGPROF";
    case SIGWINCH:
      return "SIGWINCH";
    case SIGIO:
      return "SIGIO";
    case SIGSYS:
      return "SIGSYS";
    default: {
#if !defined(__APPLE__)
      if (sig == SIGPWR) {
        return "SIGPWR";
      }
      if (sig == SIGSTKFLT) {
        return "SIGSTKFLT";
      }
      if (sig == SIGRTMAX - 0) {
        return "SIGRTMAX";
      }
      if (sig == SIGRTMAX - 1) {
        return "SIGRTMAX-1";
      }
      if (sig == SIGRTMAX - 2) {
        return "SIGRTMAX-2";
      }
      if (sig == SIGRTMAX - 3) {
        return "SIGRTMAX-3";
      }
      if (sig == SIGRTMAX - 4) {
        return "SIGRTMAX-4";
      }
      if (sig == SIGRTMAX - 5) {
        return "SIGRTMAX-5";
      }
      if (sig == SIGRTMAX - 5) {
        return "SIGRTMAX-5";
      }
      if (sig == SIGRTMAX - 6) {
        return "SIGRTMAX-6";
      }
      if (sig == SIGRTMAX - 7) {
        return "SIGRTMAX-7";
      }
      if (sig == SIGRTMAX - 8) {
        return "SIGRTMAX-8";
      }
      if (sig == SIGRTMAX - 9) {
        return "SIGRTMAX-9";
      }
#endif
      break;
    }
  }
  return strsignal(sig);
}

static void print_killing_process_description(int pid) {
  static char buf[256];
  snprintf(buf, sizeof(buf), "pid = %d", pid);
  write_to_stderr(buf);
  snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
  int fd = open(buf, O_RDONLY);
  if (fd > 0) {
    int size = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    buf[size] = 0;
    if (size > 0) {
      write_to_stderr(" (");
      write_to_stderr(buf);
      write_to_stderr(")");
    }
  }
}

static void generic_debug_handler(int sig, siginfo_t *info, void *ucontext) {
  ksignal(sig, SIG_DFL);
  if (sig == SIGABRT) {
    dump_stats();
  }
  const auto *name = signal_shortname(sig);
  write_to_stderr(name, true);
  if (info->si_code == SI_USER) {
    write_to_stderr(" received from process ");
    print_killing_process_description(info->si_pid);
    write_to_stderr(", terminating program\n", info->si_pid);
  } else {
    write_to_stderr(" caught, terminating program\n");
  }
  if (sig == SIGABRT && errno) {
    static char buf[256];
    snprintf(buf, sizeof(buf), "errno = %m\n");
    kwrite(STDERR_FILENO, buf, strlen(buf));
  }
  crash_dump_write(ucontext);
  print_backtrace();
  kill_main();
  _exit(EXIT_FAILURE);
}

static void ksignal_ext(int sig, void (*handler)(int), int sa_flags) {
  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = sa_flags;
  act.sa_handler = handler;

  if (sigaction(sig, &act, nullptr) != 0) {
    kwrite(STDERR_FILENO, "failed sigaction\n", 17);
    _exit(EXIT_FAILURE);
  }
}

static void ksignal_ext(int sig, void (*info)(int, siginfo_t *, void *), int sa_flags) {
  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = sa_flags | SA_SIGINFO;
  act.sa_sigaction = info;

  if (sigaction(sig, &act, nullptr) != 0) {
    static const char err_msg[] = "failed sigaction\n";
    kwrite(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
    _exit(EXIT_FAILURE);
  }
}

// can be called inside signal handler
void ksignal(int sig, void (*handler)(int)) {
  ksignal_ext(sig, handler, SA_ONSTACK | SA_RESTART);
}

void ksignal_intr(int sig, void (*handler)(int)) {
  ksignal_ext(sig, handler, SA_ONSTACK);
}

void ksignal(int sig, void (*info)(int, siginfo_t *, void *)) {
  ksignal_ext(sig, info, SA_ONSTACK | SA_RESTART);
}

void ksignal_intr(int sig, void (*info)(int, siginfo_t *, void *)) {
  ksignal_ext(sig, info, SA_ONSTACK);
}

void set_debug_handlers() {
  stack_t stack;
  int res = sigaltstack(nullptr, &stack);
  if (res < 0) {
    perror("sigaltstack get");
    _exit(EXIT_FAILURE);
  }
  if (!stack.ss_sp || (stack.ss_flags == SS_DISABLE)) {
    stack.ss_size = 1 << 20;
    stack.ss_sp = valloc(stack.ss_size);
    stack.ss_flags = 0;
    if (sigaltstack(&stack, nullptr) < 0) {
      perror("sigaltstack set");
      _exit(0);
    }
  }

  ksignal(SIGSEGV, generic_debug_handler);
  ksignal(SIGABRT, generic_debug_handler);
  ksignal(SIGFPE, generic_debug_handler);
  ksignal(SIGBUS, generic_debug_handler);
  debug_main_pthread_id = pthread_self();
}

volatile long long pending_signals;

static void generic_immediate_stop_handler(int sig, siginfo_t *info, void *) {
  write_to_stderr(signal_shortname(sig), true);
  if (info->si_pid && info->si_pid != getpid()) {
    write_to_stderr(" received from");
    print_killing_process_description(info->si_pid);
  } else {
    write_to_stderr(" received");
  }
  write_to_stderr(" and handled immediately.\n");
  _exit(EXIT_FAILURE);
}

static void generic_stop_handler(int sig, siginfo_t *info, void *) {
  write_to_stderr(signal_shortname(sig), true);
  if (info->si_pid && info->si_pid != getpid()) {
    write_to_stderr(" received from process ");
    print_killing_process_description(info->si_pid);
  } else {
    write_to_stderr(" received");
  }
  write_to_stderr(".\n");
  pending_signals = pending_signals | (1 << sig);
  ksignal(sig, generic_immediate_stop_handler);
}

static volatile int signals_cnt[65];

static void count_signal(int sig) {
  __sync_fetch_and_add(&signals_cnt[sig], 1);
}

static void generic_counting_handler(int sig, siginfo_t *info, void *) {
  write_to_stderr("got ", true);
  write_to_stderr(signal_shortname(sig));
  if (info->si_pid && info->si_pid != getpid()) {
    write_to_stderr(" from process ");
    print_killing_process_description(info->si_pid);
  }
  write_to_stderr(".\n");
  count_signal(sig);
}

int is_signal_pending(int sig) {
  return __sync_fetch_and_and(&signals_cnt[sig], 0) != 0;
}

void setup_delayed_handlers() {
  ksignal(SIGINT, generic_stop_handler);
  ksignal(SIGTERM, generic_stop_handler);

  ksignal(SIGPIPE, empty_handler);
  ksignal(SIGPOLL, empty_handler);

  if (daemonize) {
    ksignal(SIGHUP, generic_counting_handler);
  }
}
