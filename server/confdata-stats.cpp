// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/confdata-stats.h"

#include "common/algorithms/contains.h"

namespace {

double to_seconds(std::chrono::nanoseconds t) noexcept {
  return std::chrono::duration<double>(t).count();
}

void write_event_stats(stats_t *stats, const char *name, const ConfdataStats::EventCounters::Event &event) noexcept {
  add_gauge_stat(stats, event.total, name, ".total");
  add_gauge_stat(stats, event.blacklisted, name, ".blacklisted");
  add_gauge_stat(stats, event.ignored, name, ".ignored");
  add_gauge_stat(stats, event.ttl_updated, name, ".ttl_updated");
}

} // namespace

void ConfdataStats::on_update(const confdata_sample_storage &new_confdata,
                              size_t previous_garbage_size,
                              const ConfdataPredefinedWildcards &confdata_predefined_wildcards) noexcept {
  last_garbage_size = previous_garbage_size;
  garbage_statistic_[(total_updates++) % garbage_statistic_.size()] = last_garbage_size;

  total_elements = 0;
  simple_key_elements = 0;
  one_dot_wildcards = 0;
  one_dot_wildcard_elements = 0;
  two_dots_wildcards = 0;
  two_dots_wildcard_elements = 0;
  predefined_wildcards = 0;
  predefined_wildcard_elements = 0;
  for (const auto &section: new_confdata) {
    const vk::string_view first_key{section.first.c_str(), section.first.size()};
    switch (confdata_predefined_wildcards.detect_first_key_type(first_key)) {
      case ConfdataFirstKeyType::simple_key:
        ++simple_key_elements;
        ++total_elements;
        break;
      case ConfdataFirstKeyType::one_dot_wildcard: {
        assert(section.second.is_array());
        ++one_dot_wildcards;
        one_dot_wildcard_elements += section.second.as_array().count();
        if (!confdata_predefined_wildcards.has_wildcard_for_key(first_key)) {
          total_elements += section.second.as_array().count();
        }
        break;
      }
      case ConfdataFirstKeyType::two_dots_wildcard:
        assert(section.second.is_array());
        ++two_dots_wildcards;
        two_dots_wildcard_elements += section.second.as_array().count();
        break;
      case ConfdataFirstKeyType::predefined_wildcard: {
        assert(section.second.is_array());
        ++predefined_wildcards;
        if (confdata_predefined_wildcards.is_most_common_predefined_wildcard(first_key)) {
          predefined_wildcard_elements += section.second.as_array().count();
          if (!vk::contains(first_key, ".")) {
            total_elements += section.second.as_array().count();  
          }
        }
        break;
      }
    }
  }

  last_update_time_point = std::chrono::steady_clock::now();
}

void ConfdataStats::write_stats_to(stats_t *stats, const memory_resource::MemoryStats &memory_stats) noexcept {
  memory_stats.write_stats_to(stats, "confdata");

  add_gauge_stat_double(stats, "confdata.initial_loading_duration", to_seconds(initial_loading_time));
  add_gauge_stat_double(stats, "confdata.total_updating_time", to_seconds(total_updating_time));
  add_gauge_stat_double(stats, "confdata.seconds_since_last_update",
                            to_seconds(std::chrono::steady_clock::now() - last_update_time_point));

  add_gauge_stat_long(stats, "confdata.updates.ignored", ignored_updates);
  add_gauge_stat_long(stats, "confdata.updates.total", total_updates);

  add_gauge_stat_long(stats, "confdata.elements.total", total_elements);
  add_gauge_stat_long(stats, "confdata.elements.simple_key", simple_key_elements);
  add_gauge_stat_long(stats, "confdata.elements.one_dot_wildcard", one_dot_wildcard_elements);
  add_gauge_stat_long(stats, "confdata.elements.two_dots_wildcard", two_dots_wildcard_elements);
  add_gauge_stat_long(stats, "confdata.elements.predefined_wildcard", predefined_wildcard_elements);
  add_gauge_stat_long(stats, "confdata.elements.with_delay", elements_with_delay);

  add_gauge_stat_long(stats, "confdata.wildcards.one_dot", one_dot_wildcards);
  add_gauge_stat_long(stats, "confdata.wildcards.two_dots", two_dots_wildcards);
  add_gauge_stat_long(stats, "confdata.wildcards.predefined", predefined_wildcards);

  size_t last_100_garbage_max = 0;
  double last_100_garbage_avg = 0;
  auto garbage_last = garbage_statistic_.cbegin() + std::min(garbage_statistic_.size(), total_updates);
  for (auto it = garbage_statistic_.cbegin(); it != garbage_last; ++it) {
    last_100_garbage_max = std::max(last_100_garbage_max, *it);
    last_100_garbage_avg += static_cast<double>(*it);
  }
  if (garbage_statistic_.cbegin() != garbage_last) {
    last_100_garbage_avg /= static_cast<double>(garbage_last - garbage_statistic_.cbegin());
  }

  add_gauge_stat_long(stats, "confdata.vars_in_garbage_last", last_garbage_size);
  add_gauge_stat_long(stats, "confdata.vars_in_garbage_last_100_max", last_100_garbage_max);
  add_gauge_stat_double(stats, "confdata.vars_in_garbage_last_100_avg", last_100_garbage_avg);

  write_event_stats(stats, "confdata.binlog_events.snapshot_entry", event_counters.snapshot_entry);

  write_event_stats(stats, "confdata.binlog_events.delete", event_counters.delete_events);
  write_event_stats(stats, "confdata.binlog_events.touch", event_counters.touch_events);

  write_event_stats(stats, "confdata.binlog_events.add", event_counters.add_events);
  write_event_stats(stats, "confdata.binlog_events.set", event_counters.set_events);
  write_event_stats(stats, "confdata.binlog_events.replace", event_counters.replace_events);

  write_event_stats(stats, "confdata.binlog_events.add_forever", event_counters.add_forever_events);
  write_event_stats(stats, "confdata.binlog_events.set_forever", event_counters.set_forever_events);
  write_event_stats(stats, "confdata.binlog_events.replace_forever", event_counters.replace_forever_events);

  add_gauge_stat_long(stats, "confdata.binlog_events.get", event_counters.get_events);
  add_gauge_stat_long(stats, "confdata.binlog_events.incr", event_counters.incr_events);
  add_gauge_stat_long(stats, "confdata.binlog_events.incr_tiny", event_counters.incr_tiny_events);
  add_gauge_stat_long(stats, "confdata.binlog_events.append", event_counters.append_events);
  add_gauge_stat_long(stats, "confdata.binlog_events.unsupported_total", event_counters.unsupported_total_events);
}
