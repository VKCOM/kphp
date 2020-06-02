#include "compiler/threading/profiler.h"

#include <algorithm>
#include <cxxabi.h>
#include <unordered_map>
#include <vector>

#include "common/termformat/termformat.h"
#include "common/wrappers/fmt_format.h"

TLS<std::list<ProfilerRaw>> profiler;

std::vector<ProfilerRaw> collect_profiler_stats() {
  std::unordered_map<std::string, ProfilerRaw> collected;

  for (int i = 0; i <= MAX_THREADS_COUNT; i++) {
    const auto &local = profiler.get(i);
    for (const ProfilerRaw &raw : local) {
      auto it = collected.find(raw.name);
      if (it == collected.end()) {
        collected.emplace(raw.name, raw);
      } else {
        it->second += raw;
      }
    }
  }

  std::vector<ProfilerRaw> all;
  all.reserve(collected.size());
  std::transform(collected.begin(), collected.end(), std::back_inserter(all),
                 [](const std::pair<std::string, ProfilerRaw> &entry) {
                   return entry.second;
                 });
  std::sort(all.begin(), all.end(), [](const ProfilerRaw &a, const ProfilerRaw &b) {
    return a.print_id < b.print_id;
  });
  return all;
}

std::string pretty_memory(int64_t mem) {
  std::string res;
  TermStringFormat::color color = TermStringFormat::white;
  if (labs(mem) > 1024 * 1024 * 1024) {
    color = mem > 0 ? TermStringFormat::red : TermStringFormat::green;
    res = fmt_format("{: >+7.3f} GB", static_cast<double>(mem) / (1024.0 * 1024.0 * 1024.0));
  } else if (labs(mem) > 1024 * 1024) {
    color = mem > 0 ? TermStringFormat::yellow : TermStringFormat::cyan;
    res = fmt_format("{: >+7.1f} MB", static_cast<double>(mem) / (1024.0 * 1024.0));
  } else if (labs(mem) > 1024) {
    res = fmt_format("{: >+7.1f} KB", static_cast<double>(mem) / 1024.0);
  } else {
    color = TermStringFormat::grey;
    res = mem ? fmt_format("{: >+7d} B ", mem) : "         -";
  }
  return TermStringFormat::paint(res, color);
}

std::string pretty_time(std::chrono::nanoseconds t) {
  if (std::chrono::milliseconds{1} > t) {
    return TermStringFormat::paint(" > 0.001 sec", TermStringFormat::grey);
  }
  const auto color = t > std::chrono::seconds{10}
                     ? TermStringFormat::red
                     : t > std::chrono::seconds{1}
                       ? TermStringFormat::yellow
                       : TermStringFormat::green;
  t -= t % std::chrono::milliseconds{1};
  return TermStringFormat::paint(fmt_format("{: >8.3f} sec", std::chrono::duration<double>(t).count()), color);
}

void profiler_print_all(const std::vector<ProfilerRaw> &all) {
  size_t name_width = 0;
  for (const auto &prof : all) {
    if (prof.get_count() > 0 && prof.get_working_time().count() > 0) {
      name_width = std::max(name_width, prof.name.size());
    }
  }

  name_width += 2;
  // Name (longest_name) | Calls (9) | Working time (14) | Life time (14) | Memory (12)
  size_t table_fixed_size = 1 + 9 + 1 + 14 + 1 + 14 + 1 + 12;
  fmt_fprintf(stderr,
              "-{2:-^{0}}-\n"
              "|{3: ^{1}}|{4: ^9}|{5: ^14}|{6: ^14}|{7: ^12}|\n"
              "-{2:-^{0}}-\n",
              name_width + table_fixed_size, name_width,
              "", "Name", "Calls", "Working time", "Duration", "Memory");

  for (const auto &prof : all) {
    if (prof.get_count() > 0 && prof.get_working_time().count() > 0) {
      fmt_fprintf(stderr,
                  "|{1: ^{0}}|{2: >8} | {3: >12} | {4: >12} | {5: >10} |\n",
                  name_width,
                  prof.name,
                  prof.get_count(),
                  pretty_time(prof.get_working_time()),
                  pretty_time(prof.get_duration()),
                  pretty_memory(prof.get_memory_allocated())
      );
    }
  }
  fmt_fprintf(stderr, "-{0:-^{1}}-\n", "", name_width + table_fixed_size);
}

ProfilerRaw &get_profiler(const std::string &name) {
  static int print_id_gen;
  auto &local = *profiler;
  for (ProfilerRaw &raw : local) {
    if (raw.name == name) {
      return raw;
    }
  }
  local.emplace_back(ProfilerRaw(name, __sync_fetch_and_add(&print_id_gen, 1)));
  return local.back();
}

std::string demangle(const char *name) {
  int status;
  char *demangled = abi::__cxa_demangle(name, 0, 0, &status);
  std::string result = demangled;
  free(demangled);
  return result;
}
