// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/threading/profiler.h"

#include <algorithm>
#include <cxxabi.h>
#include <vector>

#include "common/termformat/termformat.h"
#include "common/wrappers/fmt_format.h"

static TLS<std::unordered_map<std::string, ProfilerRaw>> profiler;

std::unordered_map<std::string, ProfilerRaw> collect_profiler_stats() {
  std::unordered_map<std::string, ProfilerRaw> collected;

  for (int i = 0; i <= MAX_THREADS_COUNT; i++) {
    for (const auto& raw : profiler.get(i)) {
      if (raw.second.get_calls() > 0) {
        collected[raw.first] += raw.second;
      }
    }
  }

  return collected;
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
  const auto color = t > std::chrono::seconds{10} ? TermStringFormat::red : t > std::chrono::seconds{1} ? TermStringFormat::yellow : TermStringFormat::green;
  t -= t % std::chrono::milliseconds{1};
  return TermStringFormat::paint(fmt_format("{: >8.3f} sec", std::chrono::duration<double>(t).count()), color);
}

void profiler_print_all(const std::unordered_map<std::string, ProfilerRaw>& collected) {
  std::vector<std::pair<std::string, ProfilerRaw>> all{collected.begin(), collected.end()};
  std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a.second.print_id < b.second.print_id; });

  size_t name_width = 0;
  for (const auto& prof : all) {
    name_width = std::max(name_width, prof.first.size());
  }

  name_width += 2;
  // Name (longest_name) | Calls (9) | Working time (14) | Duration (14) | Memory (12) | Allocated (12)
  constexpr size_t table_fixed_size = 1 + 9 + 1 + 14 + 1 + 14 + 1 + 12 + 1 + 12;
  fmt_fprintf(stderr,
              "-{2:-^{0}}-\n"
              "|{3: ^{1}}|{4: ^9}|{5: ^14}|{6: ^14}|{7: ^12}|{8: ^12}|\n"
              "-{2:-^{0}}-\n",
              name_width + table_fixed_size, name_width, "", "Name", "Calls", "Working time", "Duration", "Memory", "Allocated");

  for (const auto& prof : all) {
    fmt_fprintf(stderr, "|{1: ^{0}}|{2: >8} | {3: >12} | {4: >12} | {5: >10} | {6: >10} |\n", name_width, prof.first, prof.second.get_calls(),
                pretty_time(prof.second.get_working_time()), pretty_time(prof.second.get_duration()), pretty_memory(prof.second.get_memory_usage()),
                pretty_memory(prof.second.get_memory_total_allocated()));
  }
  fmt_fprintf(stderr, "-{0:-^{1}}-\n", "", name_width + table_fixed_size);
}

ProfilerRaw& get_profiler(const std::string& name) {
  return (*profiler)[name];
}

std::string demangle(const char* name) {
  int status;
  char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  std::string result = demangled;
  free(demangled);
  return result;
}
