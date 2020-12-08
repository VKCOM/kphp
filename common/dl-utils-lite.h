// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

#include "common/cycleclock.h"
#include "common/wrappers/likely.h"

double dl_get_utime (int clock_id);
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

void dl_assert__ (const char *expr, const char *file_name, const char *func_name,
                  int line, const char *desc, int use_perror);

#define dl_assert_impl(f, str, use_perror) \
  if (unlikely(!(f))) {\
    dl_assert__ (#f, __FILE__, __FUNCTION__, __LINE__, str, use_perror);\
  }

#define dl_assert(f, str) dl_assert_impl (f, str, 0)
#define dl_passert(f, str) dl_assert_impl (f, str, 1)
#define dl_unreachable(str) dl_assert (0, str)
#define dl_fail(str) dl_assert (0, str); exit(1);
#define dl_pcheck(cmd) dl_passert (cmd >= 0, "call failed : "  #cmd)

typedef struct {
  unsigned long long utime;
  unsigned long long stime;
  unsigned long long cutime;
  unsigned long long cstime;
  unsigned long long starttime;
} pid_info_t;

typedef struct {
  unsigned long long vm_peak;
  unsigned long long vm;
  unsigned long long rss_peak;
  unsigned long long rss;
  unsigned long long rss_file;
  unsigned long long rss_shmem;
} mem_info_t;

int get_mem_stats (pid_t pid, mem_info_t *info);
int get_pid_info (pid_t pid, pid_info_t *info);
unsigned long long get_pid_start_time (pid_t pid);
int get_cpu_total (unsigned long long *cpu_total);
