#include "server/confdata-stats.h"

namespace {

double to_seconds(std::chrono::nanoseconds t) noexcept {
  return std::chrono::duration<double>(t).count();
}

void write_if_not_zero(stats_t *stats, const char *name, size_t value, const char *suffix = "") noexcept {
  if (value) {
    char buffer[256]{0};
    const int len = snprintf(buffer, sizeof(buffer) - 1, "%s%s", name, suffix);
    assert(len > 0 && sizeof(buffer) >= static_cast<size_t>(len + 1));
    add_histogram_stat_long(stats, buffer, value);
  }
}

void write_event_stats(stats_t *stats, const char *name, const ConfdataStats::EventCounters::Event &event) noexcept {
  write_if_not_zero(stats, name, event.total, ".total");
  write_if_not_zero(stats, name, event.blacklisted, ".blacklisted");
  write_if_not_zero(stats, name, event.ignored, ".ignored");
  write_if_not_zero(stats, name, event.ttl_updated, ".ttl_updated");
}

} // namespace

void ConfdataStats::on_update(const confdata_sample_storage &new_confdata, size_t previous_garbage_size) noexcept {
  last_garbage_size = previous_garbage_size;
  garbage_statistic_[(total_updates++) % garbage_statistic_.size()] = last_garbage_size;

  total_elements = 0;
  zero_dots_first_key_elements = 0;
  one_dot_first_keys = 0;
  one_dot_first_key_elements = 0;
  two_dots_first_keys = 0;
  two_dots_first_key_elements = 0;
  for (const auto &section: new_confdata) {
    switch (std::count(section.first.c_str(), section.first.c_str() + section.first.size(), '.')) {
      case 0:
        ++zero_dots_first_key_elements;
        ++total_elements;
        break;
      case 1:
        assert(section.second.is_array());
        ++one_dot_first_keys;
        one_dot_first_key_elements += section.second.as_array().count();
        total_elements += section.second.as_array().count();
        break;
      case 2:
        assert(section.second.is_array());
        ++two_dots_first_keys;
        two_dots_first_key_elements += section.second.as_array().count();
        break;
      default:
        assert(!"Unexpected section name");
    }
  }

  last_update_time_point = std::chrono::steady_clock::now();
}

void ConfdataStats::write_stats_to(stats_t *stats, const memory_resource::MemoryStats &memory_stats) noexcept {
  memory_stats.write_stats_to(stats, "confdata");

  add_histogram_stat_double(stats, "confdata.initial_loading_duration", to_seconds(initial_loading_time));
  add_histogram_stat_double(stats, "confdata.total_updating_time", to_seconds(total_updating_time));
  add_histogram_stat_double(stats, "confdata.seconds_since_last_update",
                            to_seconds(std::chrono::steady_clock::now() - last_update_time_point));

  add_histogram_stat_long(stats, "confdata.updates.ignored", ignored_updates);
  add_histogram_stat_long(stats, "confdata.updates.total", total_updates);

  add_histogram_stat_long(stats, "confdata.elements.total", total_elements);
  add_histogram_stat_long(stats, "confdata.elements.first_key_zero_dots", zero_dots_first_key_elements);
  add_histogram_stat_long(stats, "confdata.elements.first_key_one_dot", one_dot_first_key_elements);
  add_histogram_stat_long(stats, "confdata.elements.first_key_two_dots", two_dots_first_key_elements);
  add_histogram_stat_long(stats, "confdata.elements.with_delay", elements_with_delay);

  add_histogram_stat_long(stats, "confdata.first_keys.one_dot", one_dot_first_keys);
  add_histogram_stat_long(stats, "confdata.first_keys.two_dots", two_dots_first_keys);

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

  add_histogram_stat_long(stats, "confdata.vars_in_garbage_last", last_garbage_size);
  add_histogram_stat_long(stats, "confdata.vars_in_garbage_last_100_max", last_100_garbage_max);
  add_histogram_stat_double(stats, "confdata.vars_in_garbage_last_100_avg", last_100_garbage_avg);

  write_event_stats(stats, "confdata.binlog_events.snapshot_entry", event_counters.snapshot_entry);

  write_event_stats(stats, "confdata.binlog_events.delete", event_counters.delete_events);
  write_event_stats(stats, "confdata.binlog_events.touch", event_counters.touch_events);

  write_event_stats(stats, "confdata.binlog_events.add", event_counters.add_events);
  write_event_stats(stats, "confdata.binlog_events.set", event_counters.set_events);
  write_event_stats(stats, "confdata.binlog_events.replace", event_counters.replace_events);

  write_event_stats(stats, "confdata.binlog_events.add_forever", event_counters.add_forever_events);
  write_event_stats(stats, "confdata.binlog_events.set_forever", event_counters.set_forever_events);
  write_event_stats(stats, "confdata.binlog_events.replace_forever", event_counters.replace_forever_events);

  write_if_not_zero(stats, "confdata.binlog_events.get", event_counters.get_events);
  write_if_not_zero(stats, "confdata.binlog_events.incr", event_counters.incr_events);
  write_if_not_zero(stats, "confdata.binlog_events.incr_tiny", event_counters.incr_tiny_events);
  write_if_not_zero(stats, "confdata.binlog_events.append", event_counters.append_events);
  write_if_not_zero(stats, "confdata.binlog_events.unsupported_total", event_counters.unsupported_total_events);
}
