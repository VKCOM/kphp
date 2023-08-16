// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/kprintf.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common/options.h"
#include "common/precise-time.h"
#include "common/stats/provider.h"
#include "common/wrappers/pathname.h"

typedef struct {
  const char *name;
  int *value;
} verbosity_type_t;

int verbosity;
static verbosity_type_t *verbosity_types;
static int verbosity_types_num = 0;

const char *logname;
long long log_not_too_much_total = 0;

static int kprintf_multiprocessing_mode = 0;
static __thread char mp_kprintf_buf[PIPE_BUF];

void reopen_logs_ext (int slave_mode) {
  int fd;
  fflush (stdout);
  fflush (stderr);
  if ((fd = open ("/dev/null", O_RDWR, 0)) != -1) {
    dup2 (fd, 0);
    dup2 (fd, 1);
    dup2 (fd, 2);
    if (fd > 2) {
      close (fd);
    }
  }
  if (logname && (fd = open (logname, O_WRONLY|O_APPEND|O_CREAT, 0640)) != -1) {
    dup2 (fd, 1);
    dup2 (fd, 2);
    if (fd > 2) {
      close (fd);
    }
  }
  if (!slave_mode) {
    vkprintf (1, "logs reopened.\n");
  }
}

void reopen_logs () {
  reopen_logs_ext (0);
}

int hexdump(FILE *file, const void *start, const void *end) {
  const char *ptr = reinterpret_cast<const char *>(start);
  char c;
  while (ptr < (char *)end) {
    int s = (const char *)end - ptr, i;
    if (s > 16) {
      s = 16;
    }
    fprintf(file, "%08x", (int)(ptr - (char *)start));
    for (i = 0; i < 16; i++) {
      c = ' ';
      if (i == 8) {
        fputc(' ', file);
      }
      if (i < s) {
        fprintf(file, "%c%02x", c, (unsigned char)ptr[i]);
      } else {
        fprintf(file, "%c  ", c);
      }
    }
    c = ' ';
    fprintf(file, "%c  ", c);
    for (i = 0; i < s; i++) {
      putc((unsigned char)ptr[i] < ' ' ? '.' : ptr[i], file);
    }
    putc('\n', file);
    ptr += 16;
  }
  return reinterpret_cast<const char *>(end) - reinterpret_cast<const char *>(start);
}

static inline void kwrite_print_int (char **s, const char *name, int name_len, int i) {
  if (i < 0) {
    i = INT_MAX;
  }

  *--*s = ' ';
  *--*s = ']';

  do {
    *--*s = i % 10 + '0';
    i /= 10;
  } while (i > 0);

  *--*s = ' ';

  while (--name_len >= 0) {
    *--*s = name[name_len];
  }

  *--*s = '[';
}

int kwrite (int fd, const void *buf, int count) {
  int old_errno = errno;

  constexpr int S_BUF_SIZE = 100;
  char s[S_BUF_SIZE], *s_begin = s + S_BUF_SIZE;

  kwrite_print_int (&s_begin, "time", 4, time (NULL));
  kwrite_print_int (&s_begin, "pid" , 3, getpid ());

  assert (s_begin >= s);

  int s_count = s + S_BUF_SIZE - s_begin;
  int result = s_count + count;
  while (s_count > 0) {
    errno = 0;
    int res = (int)write (fd, s_begin, (size_t)s_count);
    if (errno && errno != EINTR) {
      errno = old_errno;
      return res;
    }
    if (!res) {
      break;
    }
    if (res >= 0) {
      s_begin += res;
      s_count -= res;
    }
  }

  while (count > 0) {
    errno = 0;
    int res = (int)write (fd, buf, (size_t)count);
    if (errno && errno != EINTR) {
      errno = old_errno;
      return res;
    }
    if (!res) {
      break;
    }
    if (res >= 0) {
      buf = reinterpret_cast<const char *>(buf) + res;
      count -= res;
    }
  }

  errno = old_errno;
  return result;
#undef S_BUF_SIZE
}

void kprintf_multiprocessing_mode_enable () {
  kprintf_multiprocessing_mode = 1;
}

void kprintf_ (const char *file, int line, const char *format, ...) {
  const int old_errno = errno;
  struct tm t;
  struct timeval tv;

  file = kbasename(file);
  if (gettimeofday (&tv, NULL) || !localtime_r (&tv.tv_sec, &t)) {
    memset (&t, 0, sizeof (t));
  }

  if (kprintf_multiprocessing_mode) {
    while (1) {
      if (flock (2, LOCK_EX) < 0) {
        if (errno == EINTR) {
          continue;
        }
        errno = old_errno;
        return;
      }
      break;
    }
    int n = snprintf (mp_kprintf_buf, sizeof (mp_kprintf_buf), "[%d][%4d-%02d-%02d %02d:%02d:%02d.%06d %s %4d]\n", getpid(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (int) tv.tv_usec, file, line);
    if (n < sizeof (mp_kprintf_buf) - 1) {
      errno = old_errno;
      va_list ap;
      va_start (ap, format);
      n += vsnprintf (mp_kprintf_buf + n, sizeof (mp_kprintf_buf) - n, format, ap);
      va_end (ap);
    }
    if (n >= sizeof (mp_kprintf_buf)) {
      n = sizeof (mp_kprintf_buf) - 1;
      if (mp_kprintf_buf[n-1] != '\n') {
        mp_kprintf_buf[n++] = '\n';
      }
    }
    while (write (2, mp_kprintf_buf, n) < 0 && errno == EINTR);
    while (flock (2, LOCK_UN) < 0 && errno == EINTR);
    errno = old_errno;
  } else {
    fprintf (stderr, "[%d][%4d-%02d-%02d %02d:%02d:%02d.%06d %s %4d]\n", getpid(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (int) tv.tv_usec, file, line);
    errno = old_errno;
    va_list ap;
    va_start (ap, format);
    vfprintf (stderr, format, ap);
    va_end (ap);
  }
}

void send_message_to_assertion_chat_va_list(const char *message, va_list arg_ptr) {
  fprintf(stderr, " Assertion (just warning): ");
  vfprintf(stderr, message, arg_ptr);
  fprintf(stderr, "[time %d]\n", (int) time(NULL));
}

void send_message_to_assertion_chat(const char *message, ...) {
  va_list ap;
  va_start (ap, message);
  send_message_to_assertion_chat_va_list(message, ap);
  va_end (ap);
}

STATS_PROVIDER(logs, 2000) {
  int i;
  stats->add_general_stat("verbosity", "%d", verbosity);
  for (i = 0; i < verbosity_types_num; i++) {
    stats->add_general_stat(stat_temp_format("verbosity %s", verbosity_types[i].name), "%d", *verbosity_types[i].value);
  }
  stats->add_histogram_stat("total logged errors", log_not_too_much_total);
}

#define VERBOSITY_OPTION_SHIFT 4000
#define VERBOSITY_OPTIONS_MAX  1000

static void verbosity_option_set(int *v) {
  if (!optarg) {
    if (*v == -1) {
      *v = 0;
    }
    (*v)++;
  } else {
    *v = atoi(optarg);
  }
}

int verbosity_options_parser(int c, const char *) {
  if (VERBOSITY_OPTION_SHIFT <= c && c < VERBOSITY_OPTION_SHIFT + verbosity_types_num){
    verbosity_option_set(verbosity_types[c - VERBOSITY_OPTION_SHIFT].value);
    return 0;
  }
  return -1;
}

void register_verbosity_type_inner(const char *name, int *verbosity) {
  verbosity_types = (verbosity_type_t *)realloc(verbosity_types, (verbosity_types_num + 1) * sizeof(verbosity_types[0]));
  verbosity_type_t *new_verbosity_type = &verbosity_types[verbosity_types_num];
  memset(new_verbosity_type, 0, sizeof(*new_verbosity_type));
  new_verbosity_type->name = name;
  new_verbosity_type->value = verbosity;

  static char name_buf[256];
  snprintf(name_buf, sizeof(name_buf), "verbosity_%s", verbosity_types[verbosity_types_num].name);
  int j;
  for (j = 0; name_buf[j]; j++) {
    if (name_buf[j] == '_') {
      name_buf[j] = '-';
    }
  }
  parse_common_option(OPT_VERBOSITY, verbosity_options_parser, name_buf, optional_argument, VERBOSITY_OPTION_SHIFT + verbosity_types_num, "set %s", name_buf);
  verbosity_types_num++;
}

OPTION_PARSER_SHORT(OPT_VERBOSITY, "verbosity", 'v', optional_argument, "sets or increases verbosity level") {
  verbosity_option_set(&verbosity);
  return 0;
}


int set_verbosity_by_type(const char *name, int value) {
  int i;
  for (i = 0; i < verbosity_types_num; i++) {
    if (!strcmp(name, verbosity_types[i].name)) {
      *verbosity_types[i].value = value;
      return 1;
    }
  }
  return 0;
}

const char *ip_to_print(unsigned ip) {
  static char buf[1000];
  return inet_ntop(AF_INET, &ip, buf, sizeof(buf));
}

static int conv_ipv6_internal(const unsigned short ipv6[8], char *s, size_t buf_size) {
  int p = 0;
  p += snprintf(s + p, buf_size, "%x:", htons(ipv6[0]));
  if (!ipv6[1]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[1]));
  }
  if (!ipv6[2]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[2]));
  }
  if (!ipv6[3]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[3]));
  }
  if (!ipv6[4]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[4]));
  }
  if (!ipv6[5]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[5]));
  }
  if (!ipv6[6]) {
    p += snprintf(s + p, buf_size - p, ":");
  } else {
    p += snprintf(s + p, buf_size - p, "%x:", htons(ipv6[6]));
  }
  if (ipv6[7]) {
    p += snprintf(s + p, buf_size - p, "%x", htons(ipv6[7]));
  }
  return p;
}

const char *ipv6_to_print(const void *ipv6) {
  const size_t abuf_size = 1000;
  static char abuf[abuf_size], *ptr = abuf;
  char *res;
  if (ptr > abuf + 900) {
    ptr = abuf;
  }
  res = ptr;
  ptr += conv_ipv6_internal(static_cast<const unsigned short *>(ipv6), ptr, abuf_size - (ptr - abuf)) + 1;
  return res;
}

const char *pid_to_print(const struct process_id *pid) {
  const size_t abuf_size = 1000;
  static char abuf[abuf_size], *ptr = abuf;
  char *res;
  if (ptr > abuf + 900) {
    ptr = abuf;
  }
  res = ptr;
  ptr += snprintf(ptr, abuf_size - (ptr - abuf), "[%d.%d.%d.%d:%hu:%hu:%u]",
                  pid->ip >> 24, (pid->ip >> 16) & 0xff, (pid->ip >> 8) & 0xff, pid->ip & 0xff,
                  pid->port, pid->pid, pid->utime) + 1;
  return res;
}
