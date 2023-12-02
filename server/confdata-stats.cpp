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
  stats->add_gauge_stat_with_type_tag(name, "total", event.total);
  stats->add_gauge_stat_with_type_tag(name, "blacklisted", event.blacklisted);
  stats->add_gauge_stat_with_type_tag(name, "ignored", event.ignored);
  stats->add_gauge_stat_with_type_tag(name, "throttled_out", event.throttled_out);
  stats->add_gauge_stat_with_type_tag(name, "ttl_updated", event.ttl_updated);
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

void ConfdataStats::write_stats_to(stats_t *stats, const memory_resource::MemoryStats &memory_stats) const noexcept {
  memory_stats.write_stats_to(stats, "confdata");

  stats->add_gauge_stat("confdata.initial_loading_duration", to_seconds(initial_loading_time));
  stats->add_gauge_stat("confdata.total_updating_time", to_seconds(total_updating_time));
  stats->add_gauge_stat("confdata.seconds_since_last_update", to_seconds(std::chrono::steady_clock::now() - last_update_time_point));

  stats->add_gauge_stat("confdata.updates.ignored", ignored_updates);
  stats->add_gauge_stat("confdata.updates.total", total_updates);

  stats->add_gauge_stat("confdata.elements.total", total_elements);

  stats->add_gauge_stat_with_type_tag("confdata.elements", "simple_key", simple_key_elements);
  stats->add_gauge_stat_with_type_tag("confdata.elements", "one_dot_wildcard", one_dot_wildcard_elements);
  stats->add_gauge_stat_with_type_tag("confdata.elements", "two_dots_wildcard", two_dots_wildcard_elements);
  stats->add_gauge_stat_with_type_tag("confdata.elements", "predefined_wildcard", predefined_wildcard_elements);
  stats->add_gauge_stat_with_type_tag("confdata.elements", "with_delay", elements_with_delay);

  stats->add_gauge_stat_with_type_tag("confdata.wildcards", "one_dot", one_dot_wildcards);
  stats->add_gauge_stat_with_type_tag("confdata.wildcards", "two_dots", two_dots_wildcards);
  stats->add_gauge_stat_with_type_tag("confdata.wildcards", "predefined", predefined_wildcards);

  size_t last_100_garbage_max = 0;
  double last_100_garbage_avg = 0;
  const auto *garbage_last = garbage_statistic_.cbegin() + std::min(garbage_statistic_.size(), total_updates);
  for (const auto *it = garbage_statistic_.cbegin(); it != garbage_last; ++it) {
    last_100_garbage_max = std::max(last_100_garbage_max, *it);
    last_100_garbage_avg += static_cast<double>(*it);
  }
  if (garbage_statistic_.cbegin() != garbage_last) {
    last_100_garbage_avg /= static_cast<double>(garbage_last - garbage_statistic_.cbegin());
  }

  stats->add_gauge_stat("confdata.vars_in_garbage_last", last_garbage_size);
  stats->add_gauge_stat("confdata.vars_in_garbage_last_100_max", last_100_garbage_max);
  stats->add_gauge_stat("confdata.vars_in_garbage_last_100_avg", last_100_garbage_avg);

  write_event_stats(stats, "confdata.binlog_events.snapshot_entry", event_counters.snapshot_entry);

  write_event_stats(stats, "confdata.binlog_events.delete", event_counters.delete_events);
  write_event_stats(stats, "confdata.binlog_events.touch", event_counters.touch_events);

  write_event_stats(stats, "confdata.binlog_events.add", event_counters.add_events);
  write_event_stats(stats, "confdata.binlog_events.set", event_counters.set_events);
  write_event_stats(stats, "confdata.binlog_events.replace", event_counters.replace_events);

  write_event_stats(stats, "confdata.binlog_events.add_forever", event_counters.add_forever_events);
  write_event_stats(stats, "confdata.binlog_events.set_forever", event_counters.set_forever_events);
  write_event_stats(stats, "confdata.binlog_events.replace_forever", event_counters.replace_forever_events);

  stats->add_gauge_stat_with_type_tag("confdata.binlog_events", "get", event_counters.get_events);
  stats->add_gauge_stat_with_type_tag("confdata.binlog_events", "incr", event_counters.incr_events);
  stats->add_gauge_stat_with_type_tag("confdata.binlog_events", "incr_tiny", event_counters.incr_tiny_events);
  stats->add_gauge_stat_with_type_tag("confdata.binlog_events", "append", event_counters.append_events);
  stats->add_gauge_stat_with_type_tag("confdata.binlog_events", "unsupported_total", event_counters.unsupported_total_events);
}
