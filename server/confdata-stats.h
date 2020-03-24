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

  size_t last_garbage_size{0};
  std::array<size_t, 100> garbage_statistic_{{0}};

  size_t total_elements{0};
  size_t zero_dots_first_key_elements{0};
  size_t one_dot_first_keys{0};
  size_t one_dot_first_key_elements{0};
  size_t two_dots_first_keys{0};
  size_t two_dots_first_key_elements{0};
  size_t elements_with_delay{0};

  void on_update(const confdata_sample_storage &new_confdata, size_t previous_garbage_size) noexcept;
  void write_stats_to(stats_t *stats, const memory_resource::MemoryStats &memory_stats) noexcept;

private:
  ConfdataStats() = default;
};
