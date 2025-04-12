// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/stats/provider.h"

#include <cassert>
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

char* stats_t::normalize_key(const char* key, const char* format, const char* prefix) noexcept {
  const size_t result_start_size = 1 << 10;
  static char result_start[result_start_size];
  char* result = result_start;
  int prefix_length = snprintf(result, result_start_size, "%s", prefix);
  snprintf(result + prefix_length, result_start_size - prefix_length, format, key);
  while (*result) {
    char c = *result;
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    if (!ok) {
      *result = '_';
    }
    result++;
  }
  return result_start;
}

char* stat_temp_format(const char* format, ...) {
  static char buffer[1 << 15];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  return buffer;
}

struct memory_stat_t {
  long long vm_size;
  long long vm_rss;
  long long vm_data;
  long long vm_share;
  long long mem_free;
  long long swap_total;
  long long swap_free;
  long long swap_used;
};

static int read_whole_file(const char* filename, void* output, int olen) {
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
  } while (true);
  while (close(fd) < 0 && errno == EINTR) {
  }
  if (n < 0) {
    return -1;
  }
  if (n >= olen) {
    vkprintf(1, "%s: output buffer is too small (%d bytes).\n", __func__, olen);
    return -1;
  }
  auto* p = static_cast<unsigned char*>(output);
  p[n] = 0;
  return n;
}

static int parse_statm(const char* buf, long long* a, int m) {
  static long long page_size = -1;
  if (page_size < 0) {
    page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);
  }
  int i;
  if (m > 7) {
    m = 7;
  }
  const char* p = buf;
  char* q;
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

int am_get_memory_usage(pid_t pid, long long* a, int m) {
  char proc_filename[32];
  char buf[4096];
  assert(snprintf(proc_filename, sizeof(proc_filename), "/proc/%d/statm", (int)pid) < sizeof(proc_filename));
  if (read_whole_file(proc_filename, buf, sizeof(buf)) < 0) {
    return -1;
  }
  return parse_statm(buf, a, m);
}

static int get_memory_stats(memory_stat_t* mem_stat, int flags) {
  if (!flags) {
    return -1;
  }
  long long a[6];
  memset(mem_stat, 0, sizeof(*mem_stat));

  if (flags & am_get_memory_usage_self) {
    if (am_get_memory_usage(getpid(), a, 6) < 0) {
      return -1;
    }
    mem_stat->vm_size = a[0];
    mem_stat->vm_rss = a[1];
    mem_stat->vm_share = a[2];
    mem_stat->vm_data = a[5];
  }

  if (flags & am_get_memory_usage_overall) {
    char buf[16384], *p;
    if (read_whole_file("/proc/meminfo", buf, sizeof(buf)) < 0) {
      return -1;
    }
    vkprintf(4, "/proc/meminfo: %s\n", buf);
    char key[32], suffix[32];
    long long value = 0;
    int r = 0;
    for (p = strtok(buf, "\n"); p != NULL; p = strtok(NULL, "\n")) {
      if (sscanf(p, "%31s%lld%31s", key, &value, suffix) == 3 && !strcmp(suffix, "kB")) {
        if (!strcmp(key, "MemFree:")) {
          mem_stat->mem_free = value << 10;
          r |= 1;
        } else if (!strcmp(key, "SwapTotal:")) {
          mem_stat->swap_total = value << 10;
          r |= 2;
        } else if (!strcmp(key, "SwapFree:")) {
          mem_stat->swap_free = value << 10;
          r |= 4;
        }
      }
    }
    if (r != 7) {
      return -1;
    }
    mem_stat->swap_used = mem_stat->swap_total - mem_stat->swap_free;
  }
  return 0;
}

STATS_PROVIDER(memory, 500) {
  memory_stat_t mem_stat{};
  if (!get_memory_stats(&mem_stat, am_get_memory_usage_self)) {
    stats->add_histogram_stat("vmsize_bytes", mem_stat.vm_size);
    stats->add_histogram_stat("vmrss_bytes", mem_stat.vm_rss);
    stats->add_histogram_stat("vmshared_bytes", mem_stat.vm_share);
    stats->add_histogram_stat("vmdata_bytes", mem_stat.vm_data);
  }

  if (!get_memory_stats(&mem_stat, am_get_memory_usage_overall)) {
    stats->add_general_stat("memfree_bytes", "%lld", mem_stat.mem_free);
    stats->add_histogram_stat("swap_used_bytes", mem_stat.swap_used);
    stats->add_general_stat("swap_total_bytes", "%lld", mem_stat.swap_total);
  }
}

static std::vector<stats_provider_t>& get_registered_storage() {
  static std::vector<stats_provider_t> providers;
  return providers;
}

void register_stats_provider(stats_provider_t provider) {
  auto& providers = get_registered_storage();
  providers.push_back(provider);
  for (int i = providers.size() - 1; i > 0; i--) {
    if (providers[i - 1].priority > providers[i].priority ||
        (providers[i - 1].priority == providers[i].priority && strcmp(providers[i - 1].name, providers[i].name) > 0)) {
      std::swap(providers[i], providers[i - 1]);
    } else {
      break;
    }
  }
}

static void prepare_registered_stats(stats_t* stats, unsigned int tag_mask) {
  for (auto& provider : get_registered_storage()) {
    if (provider.tag & tag_mask) {
      provider.prepare(stats);
    }
  }
}

void prepare_common_stats_with_tag_mask(stats_t* stats, unsigned int tag_mask) {
  prepare_registered_stats(stats, tag_mask);
}

void prepare_common_stats(stats_t* stats) {
  prepare_common_stats_with_tag_mask(stats, stats_tag_mask_full);
}

static void get_cmdline(int my_pid, char* cmdline_buffer, int len) {
  const size_t cmd_line_file_size = 100;
  static char cmd_line_file[cmd_line_file_size];
  snprintf(cmd_line_file, cmd_line_file_size, "/proc/%d/cmdline", my_pid);
  FILE* f = fopen(cmd_line_file, "rb");
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

  stats->add_general_stat("command_line", "%s", cmdline_buffer);
  stats->add_general_stat("pid", "%d", my_pid);

  stats->add_general_stat("start_time", "%d", now - uptime);
  stats->add_general_stat("current_time", "%d", now);
  stats->add_histogram_stat("uptime", uptime);

  stats->add_general_stat("hostname", "%s", kdb_gethostname() ?: "failed_to_get_hostname");
}

static void resource_usage_statistics(stats_t* stats, const char* prefix, struct rusage* usage) {
  double cpu_time;

  cpu_time = usage->ru_utime.tv_sec + (usage->ru_utime.tv_usec / 1E6);
  stats->add_histogram_stat(stat_temp_format("%suser_cpu_time", prefix), cpu_time);

  cpu_time = usage->ru_stime.tv_sec + (usage->ru_stime.tv_usec / 1E6);
  stats->add_histogram_stat(stat_temp_format("%ssystem_cpu_time", prefix), cpu_time);

  stats->add_general_stat(stat_temp_format("%ssoft_page_faults", prefix), "%ld", usage->ru_minflt);
  stats->add_general_stat(stat_temp_format("%shard_page_faults", prefix), "%ld", usage->ru_majflt);

  stats->add_general_stat(stat_temp_format("%sblock_input_operations", prefix), "%ld", usage->ru_inblock);
  stats->add_general_stat(stat_temp_format("%sblock_output_operations", prefix), "%ld", usage->ru_oublock);

  stats->add_histogram_stat(stat_temp_format("%svoluntary_context_switches", prefix), usage->ru_nvcsw);
  stats->add_histogram_stat(stat_temp_format("%sivoluntary_context_switches", prefix), usage->ru_nivcsw);
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
