#pragma once

#include <cstddef>
#include <cstdio>
#include <list>
#include <string>
#include <vector>

#include "common/cycleclock.h"

#include "compiler/threading/tls.h"

#define TACT_SPEED (1e-6 / 2266.0)

class ProfilerRaw {
private:
  long long count;
  long long ticks;
  size_t memory;
  int flag;
public:
  std::string name;
  int print_id;

  explicit ProfilerRaw(std::string name, int print_id) :
    count(0),
    ticks(0),
    memory(0),
    flag(0),
    name(std::move(name)),
    print_id(print_id) {

  }

  void alloc_memory(size_t size) {
    count++;
    memory += size;
  }

  size_t get_memory() const {
    return memory;
  }

  void start() {
    if (flag == 0) {
      ticks -= cycleclock_now();
      count++;
    }
    flag++;
  }

  void finish() {
    flag--;
    if (flag == 0) {
      ticks += cycleclock_now();
    }
  }

  long long get_ticks() const {
    return ticks;
  }

  long long get_count() const {
    return count;
  }

  double get_time() const {
    return get_ticks() * TACT_SPEED;
  }

  ProfilerRaw& operator+=(const ProfilerRaw& other) {
    assert(name == other.name);
    count += other.count;
    ticks += other.ticks;
    memory += other.memory;
    print_id = std::min(print_id, other.print_id);
    return *this;
  }
};

ProfilerRaw &get_profiler(const std::string &name);

std::vector<ProfilerRaw> collect_profiler_stats();
void profiler_print_all(const std::vector<ProfilerRaw> &all);

std::string demangle(const char *name);


class CachedProfiler {
  TLS<ProfilerRaw*> raws;
  std::string name;
public:


  explicit CachedProfiler(const char *name) : name(name) {}
  explicit CachedProfiler(std::string name) : name(std::move(name)) {}
  CachedProfiler(const CachedProfiler&) = delete;
  CachedProfiler(CachedProfiler&&) = delete;
  CachedProfiler& operator=(const CachedProfiler&) = delete;
  CachedProfiler& operator=(CachedProfiler&&) = delete;
  ~CachedProfiler() = default;

  ProfilerRaw& operator*() {
    return *operator->();
  }

  ProfilerRaw* operator->() {
    auto raw = *raws;
    if (raw == nullptr) {
      *raws = raw = &get_profiler(name);
    }
    return raw;
  }
};

class AutoProfiler {
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

  AutoProfiler(const AutoProfiler &) = delete;
  AutoProfiler(AutoProfiler &&) = delete;
  AutoProfiler& operator=(const AutoProfiler&) = delete;
  AutoProfiler& operator=(AutoProfiler&&) = delete;
};

