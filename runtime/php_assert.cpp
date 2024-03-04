// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/php_assert.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>

#include "common/dl-utils-lite.h"
#include "common/fast-backtrace.h"
#include "common/wrappers/pathname.h"

#include "runtime/critical_section.h"
#include "runtime/exception.h"
#include "runtime/kphp-backtrace.h"
#include "runtime/kphp_tracing.h"
#include "runtime/on_kphp_warning_callback.h"
#include "runtime/resumable.h"
#include "runtime/interface.h"

#include "server/json-logger.h"
#include "server/php-engine-vars.h"

const char *engine_tag = "[";
const char *engine_pid = "] ";

int php_disable_warnings = 0;
int php_warning_level = 2;
int php_warning_minimum_level = 0;

// linker magic: run_scheduler function is declared in separate section.
// their addresses could be used to check if address is inside run_scheduler
struct nothing {};
extern nothing __start_run_scheduler_section;
extern nothing __stop_run_scheduler_section;
static bool is_address_inside_run_scheduler(void *address) {
  return &__start_run_scheduler_section <= address && address <= &__stop_run_scheduler_section;
};

static void print_demangled_adresses(void **buffer, int nptrs, int num_shift, bool allow_gdb) {
  if (php_warning_level == 1) {
    for (int i = 0; i < nptrs; i++) {
      fprintf(stderr, "%p\n", buffer[i]);
    }
  } else if (php_warning_level == 2) {
    KphpBacktrace demangler{buffer, nptrs};
    int32_t index = num_shift;
    auto demangled_range  = demangler.make_demangled_backtrace_range(true);
    for (const char *line : demangled_range) {
      if (line) {
        fprintf(stderr, "(%d) %s", index++, line);
      }
    }
    if (index == num_shift) {
      backtrace_symbols_fd(buffer, nptrs, 2);
    }
  } else if (php_warning_level == 3 && allow_gdb) {
    const size_t pid_buf_size = 30;
    char pid_buf[pid_buf_size];
    snprintf(pid_buf, pid_buf_size, "%d", getpid());
    char name_buf[512];
    ssize_t res = readlink("/proc/self/exe", name_buf, 511);
    if (res >= 0) {
      name_buf[res] = 0;
      int child_pid = fork();
      if (!child_pid) {
        dup2(2, 1); //redirect output to stderr
        execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, nullptr);
        fprintf(stderr, "Can't print backtrace with gdb: gdb failed to start\n");
      } else {
        if (child_pid > 0) {
          waitpid(child_pid, nullptr, 0);
        } else {
          fprintf(stderr, "Can't print backtrace with gdb: fork failed\n");
        }
      }
    } else {
      fprintf(stderr, "Can't print backtrace with gdb: can't get name of executable file\n");
    }
  }
}

static void php_warning_impl(bool out_of_memory, int error_type, char const *message, va_list args) {
  if (php_warning_level == 0 || php_disable_warnings) {
    return;
  }

  const auto malloc_replacer_rollback = temporary_rollback_malloc_replacement();

  static const int BUF_SIZE = 1000;
  static char buf[BUF_SIZE];
  static const int64_t warnings_time_period = 300;
  static const int warnings_time_limit = 1000;

  static int warnings_printed = 0;
  static int64_t warnings_count_time = 0;
  static int skipped = 0;
  const int64_t cur_time = time(nullptr);

  if (cur_time >= warnings_count_time + warnings_time_period) {
    warnings_printed = 0;
    warnings_count_time = cur_time;
    if (skipped > 0) {
      fprintf(stderr, "[time=%" PRIi64 "] Resuming writing warnings: %d skipped\n", cur_time, skipped);
      skipped = 0;
    }
  }

  if (++warnings_printed >= warnings_time_limit) {
    if (warnings_printed == warnings_time_limit) {
      fprintf(stderr, "[time=% " PRIi64 "] Warnings limit reached. No more will be printed till %" PRIi64 "\n",
              cur_time, warnings_count_time + warnings_time_period);
    }
    ++skipped;
    return;
  }

  const bool allocations_allowed = dl::script_allocator_enabled && !out_of_memory && !dl::in_critical_section;
  dl::enter_critical_section();//OK

  vsnprintf(buf, BUF_SIZE, message, args);
  fprintf(stderr, "%s%" PRIi64 "%sWarning: %s\n", engine_tag, cur_time, engine_pid, buf);

  int nptrs = 0;
  void *buffer[64];
  fprintf(stderr, "------- Stack Backtrace -------\n");
  nptrs = fast_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
  if (php_warning_level == 1) {
    nptrs = std::max(nptrs - 2, 0);
  }

  int scheduler_id = static_cast<int>(std::find_if(buffer, buffer + nptrs, is_address_inside_run_scheduler) - buffer);
  if (scheduler_id == nptrs) {
    print_demangled_adresses(buffer, nptrs, 0, true);
  } else {
    print_demangled_adresses(buffer, scheduler_id, 0, true);
    void *buffer2[64];
    int res_ptrs = get_resumable_stack(buffer2, sizeof(buffer2) / sizeof(buffer2[0]));
    print_demangled_adresses(buffer2, res_ptrs, scheduler_id, false);
    print_demangled_adresses(buffer + scheduler_id, nptrs - scheduler_id, scheduler_id + res_ptrs, false);
  }

  fprintf(stderr, "-------------------------------\n\n");

  dl::leave_critical_section();
  if (allocations_allowed) {
    string warning_message = string(buf);
    OnKphpWarningCallback::get().invoke_callback(warning_message);
    if (kphp_tracing::is_turned_on()) {
      kphp_tracing::on_php_script_warning(warning_message);
    }
  }

  if (is_demangled_stacktrace_logs_enabled) {
    vk::singleton<JsonLogger>::get().write_log_with_demangled_backtrace(
      buf, error_type, cur_time, buffer, nptrs, out_of_memory || die_on_fail || error_type == E_ERROR);
  } else {
    vk::singleton<JsonLogger>::get().write_log(buf, error_type, cur_time, buffer, nptrs, out_of_memory || die_on_fail || error_type == E_ERROR);
  }

  if (die_on_fail) {
    raise_php_assert_signal__();
    fprintf(stderr, "_exiting in php_warning, since such option is enabled\n");
    _exit(1);
  }
}

void php_notice(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(false, E_NOTICE, message, args);
  va_end(args);
}

void php_warning(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(false, E_WARNING, message, args);
  va_end(args);
}

void php_error(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(false, E_ERROR, message, args);
  va_end(args);
}

void php_out_of_memory_warning(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(true, E_ERROR, message, args);
  va_end(args);
}

const char *php_uncaught_exception_error(const class_instance<C$Throwable> &ex) noexcept {
  const int64_t current_time = time(nullptr);
  const char *message = ex->$message.empty() ? "(empty)" : ex->$message.c_str();
  const char *src_file = kbasename(ex->$file.c_str());
  vk::string_view log_message = dl_pstr("Unhandled %s from %s:%" PRIi64 "; Error %" PRIi64 "; Message: %s", ex->get_class(), src_file, ex->$line, ex->$code, message);
  if (is_demangled_stacktrace_logs_enabled) {
    vk::singleton<JsonLogger>::get().write_log_with_demangled_backtrace(log_message, E_ERROR, current_time,
                                                                        ex->raw_trace.get_const_vector_pointer(), ex->raw_trace.count(), true);
  } else {
    vk::singleton<JsonLogger>::get().write_log(log_message, E_ERROR, current_time,
                                               ex->raw_trace.get_const_vector_pointer(), ex->raw_trace.count(), true);
  }

  const char *msg = dl_pstr("%s%" PRIi64 "%sError %" PRIi64 ": %s.\nUnhandled %s caught in file %s at line %" PRIi64 ".\n"
                            "Backtrace:\n%s",
                            engine_tag, current_time, engine_pid,
                            ex->$code, message, ex->get_class(), src_file, ex->$line,
                            exception_trace_as_string(ex).c_str());
  fprintf(stderr, "%s", msg);
  fprintf(stderr, "-------------------------------\n\n");
  return msg;
}

void php_assert__(const char *msg, const char *file, int line) {
  php_error("Assertion \"%s\" failed in file %s on line %d", msg, file, line);
  raise_php_assert_signal__();
  fprintf(stderr, "_exiting in php_assert\n");
  _exit(1);
}

void raise_php_assert_signal__() {
  raise(SIGPHPASSERT);
  vk::singleton<JsonLogger>::get().fsync_log_file();
}
