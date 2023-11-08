// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

#include "common/cycleclock.h"
#include "common/wrappers/likely.h"

double dl_time ();

sigset_t dl_get_empty_sigset ();
void dl_sigaction (int sig, void (*handler) (int), sigset_t mask, int flags, void (*action) (int, siginfo_t *, void *));
void dl_signal (int sig, void (*handler) (int));
void dl_restore_signal_mask ();
void dl_block_all_signals ();
void dl_allow_all_signals ();

void dl_print_backtrace(void **trace, int trace_size);
void dl_print_backtrace();
void dl_print_backtrace_gdb();

void dl_set_default_handlers ();

char* dl_pstr (char const *message, ...) __attribute__ ((format (printf, 1, 2)));

const char *dl_get_assert_message() noexcept;

void dl_assert__ (const char *expr, const char *file_name, const char *func_name,
                  int line, const char *desc, int use_perror, int generate_coredump);

#define dl_assert_impl(f, str, use_perror, generate_coredump) \
  if (unlikely(!(f))) {\
    dl_assert__ (#f, __FILE__, __FUNCTION__, __LINE__, str, use_perror, generate_coredump);\
  }

#define dl_assert(f, str) dl_assert_impl (f, str, 0, 0)
#define dl_passert(f, str) dl_assert_impl (f, str, 1, 0)
#define dl_cassert(f, str) dl_assert_impl(f, str, 0, 1)
#define dl_unreachable(str) dl_assert (0, str)

struct pid_info_t {
  unsigned long long utime;
  unsigned long long stime;
  unsigned long long cutime;
  unsigned long long cstime;
  unsigned long long starttime;
};

struct mem_info_t {
  uint32_t vm_peak;
  uint32_t vm;
  uint32_t rss_peak;
  uint32_t rss;
  uint32_t rss_file;
  uint32_t rss_shmem;
};

struct process_rusage_t {
  double user_time;
  double system_time;
  long voluntary_context_switches;
  long involuntary_context_switches;
};

mem_info_t get_self_mem_stats();
int get_self_threads_count();
int get_pid_info (pid_t pid, pid_info_t *info);
unsigned long long get_pid_start_time (pid_t pid);
int get_cpu_total (unsigned long long *cpu_total);
process_rusage_t get_rusage_info();

