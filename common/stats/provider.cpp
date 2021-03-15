// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/stats/provider.h"

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>

#include "common/kprintf.h"
#include "common/precise-time.h"
#include "common/resolver.h"

char *statsd_normalize_key(const char *key) {
  static char result_start[1 << 10];
  char *result = result_start;
  sprintf(result, "%s", key);
  while (*result) {
    char c = *result;
    int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    if (!ok) {
      *result = '_';
    }
    result++;
  }
  return result_start;
}

static void add_stat(const char type, stats_t *stats, const char *key, const char *value_format, ...) {
  stats_buffer_t *sb = &stats->sb;
  va_list ap;
  va_start(ap, value_format);
  switch (stats->type) {
    case STATS_TYPE_TL:
      sb_printf(sb, "%s\t", key);
      vsb_printf(sb, value_format, ap);
      sb_printf(sb, "\n");
      break;
    case STATS_TYPE_STATSD:
      sb_printf(sb, "%s.%s:", stats->statsd_prefix, statsd_normalize_key(key));
      vsb_printf(sb, value_format, ap);
      sb_printf(sb, "|%c\n", type);
      break;
    default:
      assert(0);
  }
  va_end (ap);
}

#define add_histogram_stat_impl(stats, key, value_format, ...) add_stat('h', stats, key, value_format, ##__VA_ARGS__)

#define add_gauge_stat_impl(stats, key, value_format, ...) add_stat('g', stats, key, value_format, ##__VA_ARGS__)

void add_histogram_stat_long(stats_t *stats, const char *key, long long value) {
  add_histogram_stat_impl(stats, key, "%lld", value);
}

void add_gauge_stat_long(stats_t *stats, const char *key, long long value) {
  add_gauge_stat_impl(stats, key, "%lld", value);
}

void add_gauge_stat_double(stats_t *stats, const char *key, double value) {
  add_gauge_stat_impl(stats, key, "%.3f", value);
}

void add_histogram_stat_double(stats_t *stats, const char *key, double value) {
  add_histogram_stat_impl(stats, key, "%.3f", value);
}

void add_general_stat(stats_t *stats, const char *key, const char *value_format, ...) {
  stats_buffer_t *sb = &stats->sb;
  va_list ap;
  va_start(ap, value_format);
  switch (stats->type) {
    case STATS_TYPE_TL:
      sb_printf(sb, "%s\t", key);
      vsb_printf(sb, value_format, ap);
      sb_printf(sb, "\n");
      break;
    case STATS_TYPE_STATSD:
      // ignore it
      break;
    default:
      assert(0);
  }
  va_end (ap);
}

char *stat_temp_format(const char *format, ...) {
  static char buffer[1 << 15];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  return buffer;
}

typedef struct {
  long long vm_size;
  long long vm_rss;
  long long vm_data;
  long long vm_share;
  long long mem_free;
  long long swap_total;
  long long swap_free;
  long long swap_used;
} memory_stat_t;


static int read_whole_file(const char *filename, void *output, int olen) {
  int fd = open(filename, O_RDONLY), n = -1;
  if (fd < 0) {
    vkprintf(1, "%s: open (\"%s\", O_RDONLY) failed. %m\n", __func__, filename);
    return -1;
  }
  do {
    n = read(fd, output, olen);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      vkprintf(1, "%s: read from %s failed. %m\n", __func__, filename);
    }
    break;
  } while (1);
  while (close(fd) < 0 && errno == EINTR) {
  }
  if (n < 0) {
    return -1;
  }
  if (n >= olen) {
    vkprintf(1, "%s: output buffer is too small (%d bytes).\n", __func__, olen);
    return -1;
  }
  auto p = static_cast<unsigned char *>(output);
  p[n] = 0;
  return n;
}

static int parse_statm(const char *buf, long long *a, int m) {
  static long long page_size = -1;
  if (page_size < 0) {
    page_size = sysconf(_SC_PAGESIZE);
    assert (page_size > 0);
  }
  int i;
  if (m > 7) {
    m = 7;
  }
  const char *p = buf;
  char *q;
  errno = 0;
  for (i = 0; i < m; i++) {
    a[i] = strtoll(p, &q, 10);
    if (p == q || errno) {
      return -1;
    }
    a[i] *= page_size;
    p = q;
  }
  return 0;
}

int am_get_memory_usage(pid_t pid, long long *a, int m) {
  char proc_filename[32];
  char buf[4096];
  assert (snprintf(proc_filename, sizeof(proc_filename), "/proc/%d/statm", (int)pid) < sizeof(proc_filename));
  if (read_whole_file(proc_filename, buf, sizeof(buf)) < 0) {
    return -1;
  }
  return parse_statm(buf, a, m);
}

static int get_memory_stats(memory_stat_t *S, int flags) {
  if (!flags) {
    return -1;
  }
  long long a[6];
  memset(S, 0, sizeof(*S));

  if (flags & AM_GET_MEMORY_USAGE_SELF) {
    if (am_get_memory_usage(getpid(), a, 6) < 0) {
      return -1;
    }
    S->vm_size = a[0];
    S->vm_rss = a[1];
    S->vm_share = a[2];
    S->vm_data = a[5];
  }

  if (flags & AM_GET_MEMORY_USAGE_OVERALL) {
    char buf[16384], *p;
    if (read_whole_file("/proc/meminfo", buf, sizeof(buf)) < 0) {
      return -1;
    }
    vkprintf(4, "/proc/meminfo: %s\n", buf);
    char key[32], suffix[32];
    long long value;
    int r = 0;
    for (p = strtok(buf, "\n"); p != NULL; p = strtok(NULL, "\n")) {
      if (sscanf(p, "%31s%lld%31s", key, &value, suffix) == 3 && !strcmp(suffix, "kB")) {
        if (!strcmp(key, "MemFree:")) {
          S->mem_free = value << 10;
          r |= 1;
        } else if (!strcmp(key, "SwapTotal:")) {
          S->swap_total = value << 10;
          r |= 2;
        } else if (!strcmp(key, "SwapFree:")) {
          S->swap_free = value << 10;
          r |= 4;
        }
      }
    }
    if (r != 7) {
      return -1;
    }
    S->swap_used = S->swap_total - S->swap_free;
  }
  return 0;
}

int64_t get_vmrss() {
  memory_stat_t S;
  if (!get_memory_stats(&S, AM_GET_MEMORY_USAGE_SELF)) {
    return S.vm_rss;
  }
  return -1;
}

STATS_PROVIDER(memory, 500) {
  memory_stat_t S;
  if (!get_memory_stats(&S, AM_GET_MEMORY_USAGE_SELF)) {
    add_histogram_stat_long(stats, "vmsize_bytes", S.vm_size);
    add_histogram_stat_long(stats, "vmrss_bytes", S.vm_rss);
    add_histogram_stat_long(stats, "vmshared_bytes", S.vm_share);
    add_histogram_stat_long(stats, "vmdata_bytes", S.vm_data);
  }

  if (!get_memory_stats(&S, AM_GET_MEMORY_USAGE_OVERALL)) {
    add_general_stat(stats, "memfree_bytes", "%lld", S.mem_free);
    add_histogram_stat_long(stats, "swap_used_bytes", S.swap_used);
    add_general_stat(stats, "swap_total_bytes", "%lld", S.swap_total);
  }
}

static std::vector<stats_provider_t> &get_registered_storage() {
  static std::vector<stats_provider_t> providers;
  return providers;
}

void register_stats_provider(stats_provider_t provider) {
  auto &providers = get_registered_storage();
  providers.push_back(provider);
  for (int i = providers.size() - 1; i > 0; i--) {
    if (providers[i - 1].priority > providers[i].priority || (providers[i - 1].priority == providers[i].priority && strcmp(providers[i - 1].name, providers[i].name) > 0)) {
      std::swap(providers[i], providers[i - 1]);
    } else {
      break;
    }
  }
}

static void prepare_registered_stats(stats_t *stats, unsigned int tag_mask) {
  for (auto &provider : get_registered_storage()) {
    if (provider.tag & tag_mask) {
      provider.prepare(stats);
    }
  }
}

void prepare_common_stats_with_tag_mask(stats_t *stats, unsigned int tag_mask) {
  prepare_registered_stats(stats, tag_mask);
}

void prepare_common_stats(stats_t *stats) {
  prepare_common_stats_with_tag_mask(stats, STATS_TAG_MASK_FULL);
}

static void get_cmdline(int my_pid, char *cmdline_buffer, int len) {
  static char cmdline_file[100];
  sprintf(cmdline_file, "/proc/%d/cmdline", my_pid);
  FILE *f = fopen(cmdline_file, "rb");
  if (f) {
    int got = fread(cmdline_buffer, 1, len - 1, f);
    fclose(f);
    for (int i = 0; i < got; i++) {
      if (cmdline_buffer[i] == 0) {
        cmdline_buffer[i] = ' ';
      }
    }
    cmdline_buffer[got] = 0;
  }
  if (!cmdline_buffer[0]) {
    strcpy(cmdline_buffer, "something went wrong, while reading cmdline");
  }
}

STATS_PROVIDER(general, 0) {
  static int my_pid = 0;
  if (!my_pid) {
    my_pid = getpid();
  }
  static char cmdline_buffer[10000];
  if (!cmdline_buffer[0]) {
    get_cmdline(my_pid, cmdline_buffer, sizeof cmdline_buffer);
  }
  int uptime = get_uptime();

  add_general_stat(stats, "command_line", "%s", cmdline_buffer);
  add_general_stat(stats, "pid", "%d", my_pid);

  add_general_stat(stats, "start_time", "%d", now - uptime);
  add_general_stat(stats, "current_time", "%d", now);
  add_histogram_stat_long(stats, "uptime", uptime);

  add_general_stat(stats, "hostname", "%s", kdb_gethostname() ?: "failed_to_get_hostname");
}

static void resource_usage_statistics(stats_t *stats, const char *prefix,
                                      struct rusage *usage) {
  double cpu_time;

  cpu_time = usage->ru_utime.tv_sec + (usage->ru_utime.tv_usec / 1E6);
  add_histogram_stat_double(stats, stat_temp_format("%suser_cpu_time", prefix), cpu_time);

  cpu_time = usage->ru_stime.tv_sec + (usage->ru_stime.tv_usec / 1E6);
  add_histogram_stat_double(stats, stat_temp_format("%ssystem_cpu_time", prefix), cpu_time);

  add_general_stat(stats, stat_temp_format("%ssoft_page_faults", prefix), "%ld", usage->ru_minflt);
  add_general_stat(stats, stat_temp_format("%shard_page_faults", prefix), "%ld", usage->ru_majflt);

  add_general_stat(stats, stat_temp_format("%sblock_input_operations", prefix), "%ld", usage->ru_inblock);
  add_general_stat(stats, stat_temp_format("%sblock_output_operations", prefix), "%ld", usage->ru_oublock);

  add_histogram_stat_long(stats, stat_temp_format("%svoluntary_context_switches", prefix), usage->ru_nvcsw);
  add_histogram_stat_long(stats, stat_temp_format("%sivoluntary_context_switches", prefix), usage->ru_nivcsw);
}

STATS_PROVIDER(resource_usage, 1000) {
  struct rusage usage;

  if (!getrusage(RUSAGE_SELF, &usage)) {
    resource_usage_statistics(stats, "", &usage);
  }
#if !defined(__APPLE__)
  if (!getrusage(RUSAGE_THREAD, &usage)) {
    resource_usage_statistics(stats, "thread_", &usage);
  }
#endif
}

void sb_print_queries(stats_t *stats, const char *const desc, long long q) {
  add_general_stat(stats, desc, "%lld", q);
  add_general_stat(stats, stat_temp_format("qps %s", desc), "%.3lf", safe_div(q, get_uptime()));
}

int get_at_prefix_length(const char *key, int key_len) {
  if (key_len > 0 && key[0] == '#') {
    return 1;
  }
  int i = 0;
  if (key_len > 0 && key[0] == '!') {
    i++;
  }
  if (i < key_len && key[i] == '-') {
    i++;
  }
  int j = i;
  while (j < key_len && isdigit(key[j])) {
    j++;
  }
  if (i == j) {
    return 0;
  }
  if (j < key_len && key[j] == '@') {
    return j + 1;
  }
  return 0;
}
