#pragma once

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <list>
#include <string>
#include <vector>

#include "common/mixin/not_copyable.h"

#include "compiler/threading/tls.h"

size_t get_thread_memory_used();

class ProfilerRaw {
private:
  long long count{0};
  std::chrono::nanoseconds working_time_{std::chrono::nanoseconds::zero()};
  std::chrono::nanoseconds min_time_{std::chrono::nanoseconds::max()};
  std::chrono::nanoseconds max_time_{std::chrono::nanoseconds::min()};
  int64_t memory_allocated_{0};
  int started_{0};

public:
  std::string name;
  int print_id;

  ProfilerRaw(std::string name, int print_id) :
    name(std::move(name)),
    print_id(print_id) {

  }

  int64_t get_memory_allocated() const {
    return memory_allocated_;
  }

  void start() {
    if (started_++ == 0) {
      const auto now = std::chrono::steady_clock::now().time_since_epoch();
      working_time_ -= now;
      min_time_ = std::min(min_time_, now);
      memory_allocated_ -= get_thread_memory_used();
      count++;
    }
  }

  void finish() {
    if (--started_ == 0) {
      const auto now = std::chrono::steady_clock::now().time_since_epoch();
      working_time_ += now;
      max_time_ = std::max(now, max_time_);
      memory_allocated_ += get_thread_memory_used();
    }
  }

  std::chrono::nanoseconds get_working_time() const noexcept {
    return working_time_;
  }

  std::chrono::nanoseconds get_duration() const noexcept {
    return max_time_ - min_time_;
  }

  long long get_count() const {
    return count;
  }

  ProfilerRaw& operator+=(const ProfilerRaw& other) {
    assert(name == other.name);
    count += other.count;
    working_time_ += other.working_time_;
    min_time_ = std::min(min_time_, other.min_time_);
    max_time_ = std::max(max_time_, other.max_time_);
    memory_allocated_ += other.memory_allocated_;
    print_id = std::min(print_id, other.print_id);
    return *this;
  }
};

ProfilerRaw &get_profiler(const std::string &name);

std::vector<ProfilerRaw> collect_profiler_stats();
void profiler_print_all(const std::vector<ProfilerRaw> &all);

std::string demangle(const char *name);


class CachedProfiler : vk::not_copyable {
  TLS<ProfilerRaw *> raws;
  std::string name;

public:
  explicit CachedProfiler(std::string name) :
    name(std::move(name)) {}

  ProfilerRaw &operator*() {
    return *operator->();
  }

  ProfilerRaw *operator->() {
    auto raw = *raws;
    if (raw == nullptr) {
      *raws = raw = &get_profiler(name);
    }
    return raw;
  }
};

class AutoProfiler : vk::not_copyable {
private:
  ProfilerRaw &prof;
public:
  explicit AutoProfiler(ProfilerRaw &prof) :
    prof(prof) {
    prof.start();
  }

  ~AutoProfiler() {
    prof.finish();
  }
};

