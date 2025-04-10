// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/limits.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/options.h"

int maxconn = MAX_CONNECTIONS;

void set_maxconn(const char* arg) {
  maxconn = atoi(arg);
  if (maxconn <= 0 || maxconn > MAX_CONNECTIONS) {
    maxconn = MAX_CONNECTIONS;
  }
}

OPTION_PARSER_SHORT(OPT_NETWORK, "connections", 'c', required_argument, "sets maximal connections number") {
  set_maxconn(optarg);
  return 0;
}

int raise_proc_rlimit(int maxprocesses) {
  struct rlimit rlim;

  if (getrlimit(RLIMIT_NPROC, &rlim) != 0) {
    kprintf("failed to getrlimit number of processes: %m\n");
    return -1;
  } else {
    if (rlim.rlim_cur < maxprocesses) {
      rlim.rlim_cur = maxprocesses + 3;
    }
    if (rlim.rlim_max < rlim.rlim_cur) {
      rlim.rlim_max = rlim.rlim_cur;
    }
    if (setrlimit(RLIMIT_NPROC, &rlim) != 0) {
      kprintf("failed to set rlimit for child processes: %m. Try running as root.\n");
      return -1;
    }
  }
  return 0;
}

int file_rlimit_init() {
  const int gap = 16;
  if (getuid()) {
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
      kprintf("%s: getrlimit (RLIMIT_NOFILE) fail. %m\n", __func__);
      return -1;
    }
    if (maxconn > rlim.rlim_cur - gap) {
      maxconn = rlim.rlim_cur - gap;
    }
  } else {
    if (raise_file_rlimit(maxconn + gap) < 0) {
      kprintf("fatal: cannot raise open file limit to %d\n", maxconn + gap);
      return -1;
    }
  }

  return 0;
}

int raise_file_rlimit(int maxfiles) {
  struct rlimit rlim;

  if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
    kprintf("failed to getrlimit number of files: %m\n");
    return -1;
  } else {
    if (rlim.rlim_cur < maxfiles) {
      rlim.rlim_cur = maxfiles + 3;
    }
    if (rlim.rlim_max < rlim.rlim_cur) {
      rlim.rlim_max = rlim.rlim_cur;
    }
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
      kprintf("failed to set rlimit for open files: %m. Try running as root or requesting smaller maxconns value.\n");
      return -1;
    }
  }
  return 0;
}

int raise_stack_rlimit(int maxstack) {
  struct rlimit rlim;

  if (getrlimit(RLIMIT_STACK, &rlim) != 0) {
    kprintf("failed to getrlimit stack size: %m\n");
    return -1;
  } else {
    if (rlim.rlim_cur < maxstack) {
      rlim.rlim_cur = maxstack;
    }
    if (rlim.rlim_max < rlim.rlim_cur) {
      rlim.rlim_max = rlim.rlim_cur;
    }
    if (setrlimit(RLIMIT_STACK, &rlim) != 0) {
      kprintf("failed to set rlimit for stack size : %m. Try running as root.\n");
      return -1;
    }
  }
  return 0;
}

int set_core_dump_rlimit(long long size_limit) {
  struct rlimit rlim;
  rlim.rlim_cur = size_limit;
  rlim.rlim_max = size_limit;

  if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
    kprintf("failed to set rlimit for core dump size.\n");
    return -1;
  }
  return 0;
}

int adjust_oom_score(int oom_score_adj) {
  const size_t path_size = 64, str_size = 16;
  static char path[path_size], str[str_size];
  assert(snprintf(path, path_size, "/proc/%d/oom_score_adj", getpid()) < 64);
  int l = snprintf(str, str_size, "%d", oom_score_adj);
  assert(l <= 15);
  int fd = open(path, O_WRONLY | O_TRUNC);
  if (fd < 0) {
    kprintf("cannot write to %s : %m\n", path);
    return -1;
  }
  int w = write(fd, str, l);
  if (w < 0) {
    kprintf("cannot write to %s : %m\n", path);
    close(fd);
    return -1;
  }
  close(fd);
  return (w == l);
}

int get_pipe_max_limit() {
  FILE* f = fopen("/proc/sys/fs/pipe-max-size", "r");
  if (f == NULL) {
    kprintf("Can't close /proc/sys/fs/pipe-max-size: %m\n");
    return 0;
  }
  int res = 0;
  if (fscanf(f, "%d", &res) != 1) {
    res = 0;
  }
  if (fclose(f) != 0) {
    kprintf("Can't close /proc/sys/fs/pipe-max-size: %m\n");
  }
  return res;
}

OPTION_PARSER(OPT_GENERIC, "nice", required_argument, "sets niceness") {
  errno = 0;
  if (nice(atoi(optarg)) < 0 && errno) {
    perror("nice");
  }
  return 0;
}

OPTION_PARSER(OPT_GENERIC, "oom-score", required_argument, "adjust oom score") {
  adjust_oom_score(atoi(optarg));
  return 0;
}
