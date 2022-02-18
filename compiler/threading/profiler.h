// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>

#include "common/mixin/not_copyable.h"

#include "compiler/threading/tls.h"

size_t get_thread_memory_usage();
size_t get_thread_memory_total_allocated();

class ProfilerRaw {
private:
  size_t calls_{0};
  std::chrono::nanoseconds working_time_{std::chrono::nanoseconds::zero()};
  std::chrono::nanoseconds min_time_{std::chrono::nanoseconds::max()};
  std::chrono::nanoseconds max_time_{std::chrono::nanoseconds::min()};
  int64_t memory_usage_{0};
  size_t memory_total_allocated_{0};
  int started_{0};

public:
  uint32_t print_id{0};

  ProfilerRaw() {
    static std::atomic<uint32_t> unique_counter{0};
    print_id = unique_counter++;
  }

  int64_t get_memory_usage() const noexcept {
    return memory_usage_;
  }

  size_t get_memory_total_allocated() const noexcept {
    return memory_total_allocated_;
  }

  void start() noexcept {
    if (started_++ == 0) {
      const auto now = std::chrono::steady_clock::now().time_since_epoch();
      working_time_ -= now;
      min_time_ = std::min(min_time_, now);
      memory_usage_ -= get_thread_memory_usage();
      memory_total_allocated_ -= get_thread_memory_total_allocated();
      calls_++;
    }
  }

  void finish() noexcept {
    if (--started_ == 0) {
      const auto now = std::chrono::steady_clock::now().time_since_epoch();
      working_time_ += now;
      max_time_ = std::max(now, max_time_);
      memory_usage_ += get_thread_memory_usage();
      memory_total_allocated_ += get_thread_memory_total_allocated();
    }
  }

  std::chrono::nanoseconds get_working_time() const noexcept {
    return working_time_;
  }

  std::chrono::nanoseconds get_duration() const noexcept {
    return max_time_ - min_time_;
  }

  size_t get_calls() const noexcept {
    return calls_;
  }

  ProfilerRaw &operator+=(const ProfilerRaw &other) noexcept {
    calls_ += other.calls_;
    working_time_ += other.working_time_;
    min_time_ = std::min(min_time_, other.min_time_);
    max_time_ = std::max(max_time_, other.max_time_);
    memory_usage_ += other.memory_usage_;
    memory_total_allocated_ += other.memory_total_allocated_;
    print_id = std::min(print_id, other.print_id);
    return *this;
  }
};

ProfilerRaw &get_profiler(const std::string &name);

std::unordered_map<std::string, ProfilerRaw> collect_profiler_stats();

void profiler_print_all(const std::unordered_map<std::string, ProfilerRaw> &collected);

std::string demangle(const char *name);


class CachedProfiler : vk::not_copyable {
  TLS<ProfilerRaw *> raws_;
  std::string name_;

public:
  explicit CachedProfiler(std::string name) :
    name_(std::move(name)) {
  }

  ProfilerRaw &operator*() {
    return *operator->();
  }

  ProfilerRaw *operator->() {
    auto *raw = *raws_;
    if (raw == nullptr) {
      raw = &get_profiler(name_);
      *raws_ = raw;
    }
    return raw;
  }
};

class AutoProfiler : vk::not_copyable {
private:
  ProfilerRaw &prof_;

public:
  explicit AutoProfiler(ProfilerRaw &prof) :
    prof_(prof) {
    prof_.start();
  }

  ~AutoProfiler() {
    prof_.finish();
  }
};

