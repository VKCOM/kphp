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

#include "runtime/allocator.h"
#include "runtime/resumable.h"

const char *engine_tag = "[";
const char *engine_pid = "] ";

int php_disable_warnings = 0;
int php_warning_level = 2;

// linker magic: run_scheduler function is declared in separate section.
// their addresses could be used to check if address is inside run_scheduler
struct nothing {};
extern nothing __start_run_scheduler_section;
extern nothing __stop_run_scheduler_section;
static bool is_address_inside_run_scheduler(void *address) {
  return &__start_run_scheduler_section <= address && address <= &__stop_run_scheduler_section;
};

static void print_demangled_adresses(void **buffer, int nptrs, int num_shift) {
  if (php_warning_level == 1) {
    for (int i = 0; i < nptrs; i++) {
      fprintf(stderr, "%p\n", buffer[i]);
    }
  } else if (php_warning_level == 2) {
    char **strings = backtrace_symbols(buffer, nptrs);

    if (strings != nullptr) {
      for (int i = 0; i < nptrs; i++) {
        char *mangled_name = nullptr, *offset_begin = nullptr, *offset_end = nullptr;
        for (char *p = strings[i]; *p; ++p) {
          if (*p == '(') {
            mangled_name = p;
          } else if (*p == '+' && mangled_name != nullptr) {
            offset_begin = p;
          } else if (*p == ')' && offset_begin != nullptr) {
            offset_end = p;
            break;
          }
        }
        if (offset_end != nullptr) {
          size_t copy_name_len = offset_begin - mangled_name;
          char *copy_name = (char *)malloc(copy_name_len);
          if (copy_name != nullptr) {
            memcpy(copy_name, mangled_name + 1, copy_name_len - 1);
            copy_name[copy_name_len - 1] = 0;

            int status;
            char *real_name = abi::__cxa_demangle(copy_name, nullptr, nullptr, &status);
            if (status < 0) {
              real_name = copy_name;
            }
            fprintf(stderr, "(%d) %.*s : %s+%.*s%s\n", i + num_shift, (int)(mangled_name - strings[i]), strings[i], real_name, (int)(offset_end - offset_begin - 1), offset_begin + 1, offset_end + 1);
            if (status == 0) {
              free(real_name);
            }

            free(copy_name);
          }
        }
      }

      free(strings);
    } else {
      backtrace_symbols_fd(buffer, nptrs, 2);
    }
  } else if (php_warning_level == 3) {
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

void php_warning(char const *message, ...) {
  if (php_warning_level == 0 || php_disable_warnings) {
    return;
  }

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

  dl::enter_critical_section();//OK

  va_list args;
  va_start (args, message);

  fprintf(stderr, "%s%d%sWarning: ", engine_tag, cur_time, engine_pid);
  vfprintf(stderr, message, args);
  fprintf(stderr, "\n");
  va_end (args);

  if (php_warning_level >= 1) {
    fprintf(stderr, "------- Stack Backtrace -------\n");
    void *buffer[64];
    int nptrs = fast_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
    if (php_warning_level == 1) {
      nptrs -= 2;
      if (nptrs < 0) {
        nptrs = 0;
      }
    }

    int scheduler_id = std::find_if(buffer, buffer + nptrs, is_address_inside_run_scheduler) - buffer;
    if (scheduler_id == nptrs) {
      print_demangled_adresses(buffer, nptrs, 0);
    } else {
      print_demangled_adresses(buffer, scheduler_id, 0);
      void *buffer2[64];
      int res_ptrs = get_resumable_stack(buffer2, sizeof(buffer2) / sizeof(buffer2[0]));
      print_demangled_adresses(buffer2, res_ptrs, scheduler_id);
      print_demangled_adresses(buffer + scheduler_id, nptrs - scheduler_id, scheduler_id + res_ptrs);
    }

    fprintf(stderr, "-------------------------------\n\n");
  }

  dl::leave_critical_section();
  if (die_on_fail) {
    fprintf(stderr, "_exiting in php_warning, since such option is enabled\n");
    raise(SIGUSR2);
    _exit(1);
  }
}
