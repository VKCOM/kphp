// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/stats/buffer.h"

constexpr int am_get_memory_usage_self = 1;
constexpr int am_get_memory_usage_overall = 2;

constexpr unsigned int stats_tag_default = 1 << 0;
constexpr unsigned int stats_tag_kphp_server = 1 << 31;
constexpr unsigned int stats_tag_mask_full = 0xFFFFFFFF;

class stats_t {
public:
  stats_buffer_t sb{};
  const char* stats_prefix{};

  template<typename T>
  void add_histogram_stat(const char* key, T value) {
    static_assert(std::is_integral<T>{} || std::is_floating_point<T>{}, "integral or floating point expected");
    if constexpr (std::is_floating_point_v<T>) {
      add_stat('h', key, static_cast<double>(value));
    } else {
      add_stat('h', key, static_cast<long long>(value));
    }
  }

  template<typename T>
  void add_gauge_stat(const char* key, T value) {
    static_assert(std::is_integral<T>{} || std::is_floating_point<T>{}, "integral or floating point expected");
    if constexpr (std::is_floating_point_v<T>) {
      add_stat('g', key, static_cast<double>(value));
    } else {
      add_stat('g', key, static_cast<long long>(value));
    }
  }

  template<typename T>
  void add_gauge_stat_with_type_tag(const char* key, [[maybe_unused]] const char* tag, T value) {
    static_assert(std::is_integral<T>{} || std::is_floating_point<T>{}, "integral or floating point expected");
    if constexpr (std::is_floating_point_v<T>) {
      add_stat_with_tag_type('g', key, tag, static_cast<double>(value));
    } else {
      add_stat_with_tag_type('g', key, tag, static_cast<long long>(value));
    }
  }

  template<typename T>
  void add_gauge_stat(T value, const char* key1, const char* key2 = "", const char* key3 = "") noexcept {
    static_assert(std::is_integral<T>{} || std::is_floating_point<T>{}, "integral or floating point expected");
    const size_t key1_len = std::strlen(key1);
    const size_t key2_len = std::strlen(key2);
    const size_t key3_len = std::strlen(key3);
    char stat_key[key1_len + key2_len + key3_len + 1];
    std::memcpy(stat_key, key1, key1_len);
    std::memcpy(stat_key + key1_len, key2, key2_len);
    std::memcpy(stat_key + key1_len + key2_len, key3, key3_len + 1);
    add_gauge_stat(stat_key, value);
  }

  template<typename T>
  void add_gauge_stat(const std::atomic<T>& value, const char* key1, const char* key2 = "", const char* key3 = "") noexcept {
    add_gauge_stat(value.load(std::memory_order_relaxed), key1, key2, key3);
  }

  virtual void add_general_stat(const char* key, const char* value_format, ...) noexcept __attribute__((format(printf, 3, 4))) = 0;

  virtual ~stats_t() = default;

protected:
  virtual void add_stat(char type, const char* key, double value) noexcept = 0;
  virtual void add_stat(char type, const char* key, long long value) noexcept = 0;

  virtual void add_stat_with_tag_type(char type, const char* key, const char* type_tag, double value) noexcept = 0;
  virtual void add_stat_with_tag_type(char type, const char* key, const char* type_tag, long long value) noexcept = 0;

  char* normalize_key(const char* key, const char* format, const char* prefix) noexcept;
};

char* stat_temp_format(const char* format, ...) __attribute__((format(printf, 1, 2)));
int am_get_memory_usage(pid_t pid, long long* a, int m);

struct stats_provider_t {
  const char* name;
  int priority;     // smaller values mean early in stats
  unsigned int tag; // allows to specify tag for filtering stats
  void (*prepare)(stats_t* stats);
};

void register_stats_provider(stats_provider_t provider);
void prepare_common_stats_with_tag_mask(stats_t* stats, unsigned int tag_mask);
void prepare_common_stats(stats_t* stats);

static inline double safe_div(double x, double y) {
  return y > 0 ? x / y : 0;
}

#define STATS_PROVIDER_TAGGED(nm, prio, tag_val)                                                                                                               \
  static void prepare_##nm##_stats(stats_t* stats);                                                                                                            \
  static void register_##nm##_stats() __attribute__((constructor));                                                                                            \
  static void register_##nm##_stats() {                                                                                                                        \
    stats_provider_t provider;                                                                                                                                 \
    provider.name = #nm;                                                                                                                                       \
    provider.priority = (prio);                                                                                                                                \
    provider.tag = (tag_val);                                                                                                                                  \
    provider.prepare = prepare_##nm##_stats;                                                                                                                   \
    register_stats_provider(provider);                                                                                                                         \
  }                                                                                                                                                            \
  static void prepare_##nm##_stats(stats_t* stats)

#define STATS_PROVIDER(nm, prio) STATS_PROVIDER_TAGGED(nm, prio, stats_tag_default)
