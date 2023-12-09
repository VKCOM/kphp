// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

#include "common/stats/provider.h"
#include "runtime/confdata-global-manager.h"

struct ConfdataStats : private vk::not_copyable {
  static ConfdataStats &get() noexcept {
    static ConfdataStats confdata_stats;
    return confdata_stats;
  }

  std::chrono::nanoseconds initial_loading_time{std::chrono::nanoseconds::zero()};
  std::chrono::nanoseconds total_updating_time{std::chrono::nanoseconds::zero()};
  std::chrono::steady_clock::time_point last_update_time_point{std::chrono::nanoseconds::zero()};

  size_t total_updates{0};
  size_t ignored_updates{0};
  size_t timed_out_updates;

  size_t last_garbage_size{0};
  std::array<size_t, 100> garbage_statistic_{{0}};

  size_t total_elements{0};
  size_t simple_key_elements{0};
  size_t one_dot_wildcards{0};
  size_t one_dot_wildcard_elements{0};
  size_t two_dots_wildcards{0};
  size_t two_dots_wildcard_elements{0};
  size_t predefined_wildcards{0};
  size_t predefined_wildcard_elements{0};
  size_t elements_with_delay{0};

  struct EventCounters {
    struct Event {
      size_t total{0};
      size_t blacklisted{0};
      size_t ignored{0};
      size_t throttled_out{0};
      size_t ttl_updated{0};
    };

    Event snapshot_entry;

    Event delete_events;
    Event touch_events;

    Event add_events;
    Event set_events;
    Event replace_events;

    Event add_forever_events;
    Event set_forever_events;
    Event replace_forever_events;

    size_t get_events{0};
    size_t incr_events{0};
    size_t incr_tiny_events{0};
    size_t append_events{0};

    size_t unsupported_total_events{0};
    size_t throttled_out_total_events{0};
  } event_counters;

  struct HeaviestSections {
    static constexpr int LEN = 10;

    // store pointers, not to copy strings when sorting (they point to array keys located in shared mem)
    std::array<std::pair<const string *, size_t>, LEN> sorted_desc;

    void clear();
    void register_section(const string *section_name, size_t size);
  };

  HeaviestSections heaviest_sections_by_count;
  // in the future, we may want distribution by estimate_memory_usage():
  // HeaviestSections heaviest_sections_by_mem;

  const memory_resource::MemoryStats &get_memory_stats() const noexcept;

  void on_update(const confdata_sample_storage &new_confdata,
                 size_t previous_garbage_size,
                 const ConfdataPredefinedWildcards &predefined_wildcards) noexcept;
  void write_stats_to(stats_t *stats) const noexcept;

private:
  ConfdataStats() = default;
};
