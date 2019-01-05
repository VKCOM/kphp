#include "compiler/threading/profiler.h"

#include <algorithm>
#include <cxxabi.h>
#include <map>
#include <vector>

TLS<std::list<ProfilerRaw>> profiler;

void profiler_print_all() {
  std::map<std::string, ProfilerRaw> collected;

  for (int i = 0; i <= MAX_THREADS_COUNT; i++) {
    const auto &local = *profiler.get(i);
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
  std::transform(collected.begin(), collected.end(), std::back_inserter(all),
    [](const std::pair<std::string, ProfilerRaw> &entry) {
      return entry.second;
  });
  sort(all.begin(), all.end(), [](const ProfilerRaw &a, const ProfilerRaw & b) {
    return a.print_id < b.print_id;
  });


  for (const auto &prof : all) {
    if (prof.get_count() > 0 && prof.get_ticks() > 0) {
      fprintf(stderr, "%60s:\t%lf %lld %lld\n",
        prof.name.c_str(),
        prof.get_time(),
        prof.get_count(),
        prof.get_ticks() / std::max(1ll, prof.get_count())
      );
    }
  }

  for (const auto &prof : all) {
    if (prof.get_count() > 0 && prof.get_memory() > 0) {
      fprintf(stderr, "%60s:\t%.5lfMb %lld %.5lf\n",
        prof.name.c_str(),
        (double)prof.get_memory() / (1 << 20),
        prof.get_count(), (double)prof.get_memory() / prof.get_count()
      );
    }
  }
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
