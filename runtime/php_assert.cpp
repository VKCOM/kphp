#include "runtime/php_assert.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <wait.h>

#include "common/fast-backtrace.h"

#include "runtime/critical_section.h"
#include "runtime/datetime.h"
#include "runtime/kphp_backtrace.h"
#include "runtime/misc.h"
#include "runtime/on_kphp_warning_callback.h"
#include "runtime/resumable.h"
#include "server/php-engine-vars.h"

const char *engine_tag = "[";
const char *engine_pid = "] ";

int php_disable_warnings = 0;
int php_warning_level = 2;
int php_warning_minimum_level = 0;

extern FILE* json_log_file_ptr;

// linker magic: run_scheduler function is declared in separate section.
// their addresses could be used to check if address is inside run_scheduler
struct nothing {};
extern nothing __start_run_scheduler_section;
extern nothing __stop_run_scheduler_section;
static bool is_address_inside_run_scheduler(void *address) {
  return &__start_run_scheduler_section <= address && address <= &__stop_run_scheduler_section;
};

void write_json_error_to_log(int version, const string& msg, int type, const array<string>& cpp_trace, int nptrs, void** buffer);

array<var> f$kphp_error_callback() __attribute__((weak));
array<var> f$kphp_error_callback() {
  return {};
}

static void print_demangled_adresses(void **buffer, int nptrs, int num_shift, backtrace_each_line_callback_t callback, bool allow_gdb) {
  if (php_warning_level == 1) {
    for (int i = 0; i < nptrs; i++) {
      fprintf(stderr, "%p\n", buffer[i]);
    }
  } else if (php_warning_level == 2) {
    bool was_printed = get_demangled_backtrace(buffer, nptrs, num_shift, callback);
    if (!was_printed) {
      backtrace_symbols_fd(buffer, nptrs, 2);
    }
  } else if (php_warning_level == 3 && allow_gdb) {
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
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

static void php_warning_impl(bool out_of_memory, char const *message, va_list args) {
  if (php_warning_level == 0 || php_disable_warnings) {
    return;
  }
  static const int BUF_SIZE = 1000;
  static char buf[BUF_SIZE];
  static const int warnings_time_period = 300;
  static const int warnings_time_limit = 1000;

  static int warnings_printed = 0;
  static int warnings_count_time = 0;
  static int skipped = 0;
  int cur_time = (int)time(nullptr);

  if (cur_time >= warnings_count_time + warnings_time_period) {
    warnings_printed = 0;
    warnings_count_time = cur_time;
    if (skipped > 0) {
      fprintf(stderr, "[time=%d] Resuming writing warnings: %d skipped\n", (int)time(nullptr), skipped);
      skipped = 0;
    }
  }

  if (++warnings_printed >= warnings_time_limit) {
    if (warnings_printed == warnings_time_limit) {
      fprintf(stderr, "[time=%d] Warnings limit reached. No more will be printed till %d\n", cur_time, warnings_count_time + warnings_time_period);
    }
    ++skipped;
    return;
  }

  const bool allocations_allowed = !out_of_memory && !dl::in_critical_section;
  dl::enter_critical_section();//OK

  fprintf(stderr, "%s%d%sWarning: ", engine_tag, cur_time, engine_pid);
  vsnprintf(buf, BUF_SIZE, message, args);
  fprintf(stderr, "%s\n", buf);

  bool need_stacktrace = php_warning_level >= 1;
  int nptrs = 0;
  void *buffer[64];
  array<string> cpp_trace;
  if (need_stacktrace) {
    fprintf(stderr, "------- Stack Backtrace -------\n");
    nptrs = fast_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
    if (php_warning_level == 1) {
      nptrs -= 2;
      if (nptrs < 0) {
        nptrs = 0;
      }
    }

    auto callback = [&cpp_trace, allocations_allowed](const char *, const char *trace_str, const char *json_trace_str) {
      fprintf(stderr, "%s", trace_str);
      if (allocations_allowed) {
        cpp_trace.emplace_back(json_trace_str);
      }
    };

    int scheduler_id = std::find_if(buffer, buffer + nptrs, is_address_inside_run_scheduler) - buffer;
    if (scheduler_id == nptrs) {
      print_demangled_adresses(buffer, nptrs, 0, callback, true);
    } else {
      print_demangled_adresses(buffer, scheduler_id, 0, callback, true);
      void *buffer2[64];
      int res_ptrs = get_resumable_stack(buffer2, sizeof(buffer2) / sizeof(buffer2[0]));
      print_demangled_adresses(buffer2, res_ptrs, scheduler_id, callback, false);
      print_demangled_adresses(buffer + scheduler_id, nptrs - scheduler_id, scheduler_id + res_ptrs, callback, false);
    }

    fprintf(stderr, "-------------------------------\n\n");
  }

  dl::leave_critical_section();
  if (allocations_allowed) {
    OnKphpWarningCallback::get().invoke_callback(string(buf));

    if (need_stacktrace && json_log_file_ptr != nullptr) {
      auto version = string(engine_tag).to_int();
      write_json_error_to_log(version, string(buf), 1, cpp_trace, nptrs, buffer);
    }
  }
  if (die_on_fail) {
    raise(SIGPHPASSERT);
    fprintf(stderr, "_exiting in php_warning, since such option is enabled\n");
    _exit(1);
  }
}

void php_warning(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(false, message, args);
  va_end(args);
}

void php_out_of_memory_warning(char const *message, ...) {
  va_list args;
  va_start (args, message);
  php_warning_impl(true, message, args);
  va_end(args);
}

void php_assert__(const char *msg, const char *file, int line) {
  php_warning("Assertion \"%s\" failed in file %s on line %d", msg, file, line);
  raise(SIGPHPASSERT);
  fprintf(stderr, "_exiting in php_assert\n");
  _exit(1);
}

void raise_php_assert_signal__() {
  raise(SIGPHPASSERT);
}

// You must call this function after dashed line (old log separation line)
void write_json_error_to_log(int version, const string& msg, int type, const array<string>& cpp_trace, int nptrs, void** buffer) {
  array<string> trace;
  char buffer_for_ptr[50];
  for (int i = 0; i < nptrs; i++) {
    sprintf(buffer_for_ptr, "%p", buffer[i]);
    trace.emplace_back(string(buffer_for_ptr));
  }

  array<var> ar;
  ar.set_value(string("msg"), msg);
  ar.set_value(string("version"), version);
  ar.set_value(string("type"), type);
  ar.set_value(string("trace"), trace);
  ar.set_value(string("cpp_trace"), cpp_trace);
  ar.set_value(string("created_at"), f$time());

  // properties from callback
  auto custom_properties = f$kphp_error_callback();
  const auto tagsKey = string("tags");
  ar.set_value(tagsKey, custom_properties.get_value(tagsKey));

  const auto extraInfoKey = string("extra_info");
  ar.set_value(extraInfoKey, custom_properties.get_value(extraInfoKey));

  auto opt = f$json_encode(ar);
  if (!opt.has_value()) {
    // remember this line can break old log format
    fprintf(stderr, "%s function can't encode json\n", __FUNCTION__);
    return;
  }

  auto str = opt.val();
  fwrite(str.c_str(), sizeof(char), str.size(), json_log_file_ptr);
  fwrite("\n", sizeof(char), 1, json_log_file_ptr);
  fflush(json_log_file_ptr);
}