// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "dl-utils-lite.h"

#include <assert.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common/stats/provider.h"

#if DL_DEBUG_MEM >= 1
#  define MEM_POS  {\
  void *buffer[64]; \
  int nptrs = backtrace (buffer, 4); \
  fprintf (stderr, "\n------- Stack Backtrace -------\n"); \
  backtrace_symbols_fd (buffer + 1, nptrs - 1, 2); \
  fprintf (stderr, "-------------------------------\n"); \
}
#else
#  define MEM_POS
#endif



double dl_get_utime (int clock_id) {
  struct timespec T;
#if _POSIX_TIMERS
  assert (clock_gettime (clock_id, &T) >= 0);
  return (double)T.tv_sec + (double)T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  return (double)time();
#endif
}

#define NSEC_IN_SEC 1000000000LL
#define NSEC_IN_MSEC 1000000LL

long long dl_get_utime_ns (int clock_id) {
  struct timespec T;
#if _POSIX_TIMERS
  assert (clock_gettime (clock_id, &T) >= 0);
  return T.tv_sec * NSEC_IN_SEC + T.tv_nsec;
#else
#error "No high-precision clock"
  return time() * NSEC_IN_SEC;
#endif
}

double dl_time () {
  return dl_get_utime (CLOCK_MONOTONIC);
}

void dl_print_backtrace(void **trace, int trace_size) {
  write (2, "\n------- Stack Backtrace -------\n", 33);
  backtrace_symbols_fd (trace, trace_size, 2);
  write (2, "-------------------------------\n", 32);
}

void dl_print_backtrace() {
  void *buffer[64];
  int nptrs = backtrace (buffer, 64);
  dl_print_backtrace(buffer, nptrs);
}

void dl_print_backtrace_gdb() {
  char * const envp[] = {NULL};
  char pid_buf[30];
  sprintf (pid_buf, "%d", getpid());
  char name_buf[512];
  ssize_t res = readlink ("/proc/self/exe", name_buf, 511);
  if (res >= 0) {
    name_buf[res] = 0;
    int child_pid = fork();
    if (child_pid < 0) {
      write (2, "Can't fork() to run gdb\n", 24);
      _exit (0);
    }
    if (!child_pid) {
      dup2 (2, 1); //redirect output to stderr
      //fprintf (stdout, "stack trace for %s pid = %s\n", name_buf, pid_buf);
      execle ("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, (char *)NULL, envp);
      _exit (0); /* If gdb failed to start */
    } else {
      waitpid (child_pid, NULL, 0);
    }
  } else {
    write (2, "can't get name of executable file to pass to gdb\n", 49);
  }
}

void dl_assert__ (const char *expr __attribute__((unused)), const char *file_name, const char *func_name,
                  int line, const char *desc, int use_perror) {
  fprintf (stderr, "dl_assert failed [%s : %s : %d]: ", file_name, func_name, line);
  fprintf (stderr, "%s\n", desc);
  if (use_perror) {
    perror ("perror description");
  }
  abort();
}

static sigset_t old_mask;
static int old_mask_inited = 0;

sigset_t dl_get_empty_sigset () {
  sigset_t mask;
  sigemptyset (&mask);
  return mask;
}

void dl_sigaction (int sig, void (*handler) (int), sigset_t mask, int flags, void (*action) (int, siginfo_t *, void *)) {
  struct sigaction act;
  memset (&act, 0, sizeof (act));
  act.sa_mask = mask;
  act.sa_flags = flags;

  if (handler != NULL) {
    act.sa_handler = handler;
    assert (action == NULL);
    assert (!(flags & SA_SIGINFO));
  } else if (action != NULL) {
    flags |= SA_SIGINFO;
    act.sa_sigaction = action;
  }

  int err = sigaction (sig, &act, NULL);
  dl_passert (err != -1, "failed sigaction");
}

void dl_signal (int sig, void (*handler) (int)) {
  dl_sigaction (sig, handler, dl_get_empty_sigset(), SA_ONSTACK | SA_RESTART, NULL);
}

void dl_restore_signal_mask () {
  dl_assert (old_mask_inited != 0, "old_mask in not inited");
  int err = sigprocmask (SIG_SETMASK, &old_mask, NULL);
  dl_passert (err != -1, "failed to restore signal mask");
}

void dl_block_all_signals () {
  sigset_t mask;
  sigfillset (&mask);
  int err = sigprocmask (SIG_SETMASK, &mask, &old_mask);
  old_mask_inited = 1;
  dl_passert (err != -1, "failed to block all signals");
}

void dl_allow_all_signals () {
  sigset_t mask;
  sigemptyset (&mask);
  int err = sigprocmask (SIG_SETMASK, &mask, &old_mask);
  old_mask_inited = 1;
  dl_passert (err != -1, "failed to allow all signals");
}

static void runtime_handler (const int sig) {
  fprintf (stderr, "%s caught, terminating program\n", sig == SIGSEGV ? "SIGSEGV" : "SIGABRT");
  dl_print_backtrace();
  dl_print_backtrace_gdb();
  _exit (EXIT_FAILURE);
}

void dl_set_default_handlers () {
  dl_signal (SIGSEGV, runtime_handler);
  dl_signal (SIGABRT, runtime_handler);
}

char *dl_pstr (char const *msg, ...) {
  static char s[5000];
  va_list args;

  va_start (args, msg);
  vsnprintf (s, 5000, msg, args);
  va_end (args);

  return s;
}

/** Memory and cpu stats **/
int get_mem_stats (pid_t pid, mem_info_t *info) {
#define TMEM_SIZE 10000
  static char mem[TMEM_SIZE];
  snprintf (mem, TMEM_SIZE, "/proc/%lu/status", (unsigned long)pid);
  int fd = open (mem, O_RDONLY);

  if (fd == -1) {
    return 0;
  }

  int size = (int)read (fd, mem, TMEM_SIZE - 1);
  if (size <= 0) {
    close (fd);
    return 0;
  }
  mem[size] = 0;

  char *s = mem;
  while (*s) {
    char *st = s;
    while (*s != 0 && *s != '\n') {
      s++;
    }
    unsigned long long *x = NULL;
    if (strncmp (st, "VmPeak", 6) == 0) {
      x = &info->vm_peak;
    }
    if (strncmp (st, "VmSize", 6) == 0) {
      x = &info->vm;
    }
    if (strncmp (st, "VmHWM", 5) == 0) {
      x = &info->rss_peak;
    }
    if (strncmp (st, "VmRSS", 5) == 0) {
      x = &info->rss;
    }
    if (strncmp (st, "RssFile", 7) == 0) {
      x = &info->rss_file;
    }
    if (strncmp (st, "RssShmem", 8) == 0) {
      x = &info->rss_shmem;
    }
    if (x != NULL) {
      while (st < s && *st != ' ' && *st != '\t') {
        st++;
      }
      *x = (unsigned long long)-1;

      if (st < s) {
        sscanf (st, "%llu", x);
      }
    }
    if (*s == 0) {
      break;
    }
    s++;
  }

  close (fd);
  return 1;
#undef TMEM_SIZE
}

int get_pid_info (pid_t pid, pid_info_t *info) {
#define TMEM_SIZE 10000
  static char mem[TMEM_SIZE];
  snprintf (mem, TMEM_SIZE, "/proc/%lu/stat", (unsigned long)pid);
  int fd = open (mem, O_RDONLY);

  if (fd == -1) {
    return 0;
  }

  int size = (int)read (fd, mem, TMEM_SIZE - 1);
  if (size <= 0) {
    close (fd);
    return 0;
  }
  mem[size] = 0;

  char *s = mem;
  int pass_cnt = 0;

  while (pass_cnt < 22) {
    if (pass_cnt == 13) {
      sscanf (s, "%llu", &info->utime);
    }
    if (pass_cnt == 14) {
      sscanf (s, "%llu", &info->stime);
   }
    if (pass_cnt == 15) {
      sscanf (s, "%llu", &info->cutime);
    }
    if (pass_cnt == 16) {
      sscanf (s, "%llu", &info->cstime);
    }
    if (pass_cnt == 21) {
      sscanf (s, "%llu", &info->starttime);
    }
    while (*s && *s != ' ') {
      s++;
    }
    if (*s == ' ') {
      s++;
      pass_cnt++;
    } else {
      dl_assert (0, "unexpected end of proc file");
      break;
    }
  }

  close (fd);
  return 1;
#undef TMEM_SIZE
}

unsigned long long get_pid_start_time (pid_t pid) {
  pid_info_t info;
  unsigned long long res = 0;
  if (get_pid_info (pid, &info)) {
    res = info.starttime;
  }

  return res;
}

int get_cpu_total (unsigned long long *cpu_total) {
#define TMEM_SIZE 10000
  static char mem[TMEM_SIZE];
  snprintf (mem, TMEM_SIZE, "/proc/stat");
  int fd = open (mem, O_RDONLY);

  if (fd == -1) {
    return 0;
  }

  int size = (int)read (fd, mem, TMEM_SIZE - 1);
  if (size <= 0) {
    close (fd);
    return 0;
  }

  unsigned long long sum = 0, cur = 0;
  int i;
  for (i = 0; i < size; i++) {
    int c = mem[i];
    if (c >= '0' && c <= '9') {
      cur = cur * 10 + (unsigned long long)c - '0';
    } else {
      sum += cur;
      cur = 0;
      if (c == '\n') {
        break;
      }
    }
  }

  *cpu_total = sum;

  close (fd);
  return 1;
#undef TMEM_SIZE
}
