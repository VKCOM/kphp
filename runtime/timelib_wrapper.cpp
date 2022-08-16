#include "runtime/timelib_wrapper.h"

#include <kphp/timelib/timelib.h>
#include <sys/time.h>

#include "common/containers/final_action.h"
#include "common/smart_ptrs/singleton.h"

// these constants are a part of the private timelib API, but PHP uses them internally;
// we define them here locally
constexpr int TIMELIB_SPECIAL_WEEKDAY = 0x01;
constexpr int TIMELIB_SPECIAL_FIRST_DAY_OF_MONTH = 0x01;

namespace {

timelib_tzinfo *get_timezone_info(const char *tz_name) {
  int dummy_error_code = 0;
  timelib_tzinfo *info = timelib_parse_tzfile(tz_name, timelib_builtin_db(), &dummy_error_code);
  return info;
}

void set_time_value(array<mixed> &dst, const char *name, int64_t value) {
  // PHP7.4 and earlier version of PHP8 use false as the default value;
  // PHP master sources use false too, but some versions of PHP8
  // used 0 as default (that could be a bug in PHP, but we may need to pay attention
  // to that when making a transition to PHP8)
  if (value == TIMELIB_UNSET) {
    dst.set_value(string(name), false);
  } else {
    dst.set_value(string(name), value);
  }
}

array<mixed> dump_errors(const timelib_error_container &error) {
  array<mixed> result;

  array<string> result_warnings;
  result_warnings.reserve(error.warning_count, 0, false);
  for (int i = 0; i < error.warning_count; i++) {
    result_warnings.set_value(error.warning_messages[i].position, string(error.warning_messages[i].message));
  }
  result.set_value(string("warning_count"), error.warning_count);
  result.set_value(string("warnings"), result_warnings);

  array<string> result_errors;
  result_errors.reserve(error.error_count, 0, false);
  for (int i = 0; i < error.error_count; i++) {
    result_errors.set_value(error.error_messages[i].position, string(error.error_messages[i].message));
  }
  result.set_value(string("error_count"), error.error_count);
  result.set_value(string("errors"), result_errors);

  return result;
}

array<mixed> create_date_parse_array(timelib_time *t, timelib_error_container *error) {
  array<mixed> result;

  // note: we're setting the result array keys in the same order as PHP does

  set_time_value(result, "year", t->y);
  set_time_value(result, "month", t->m);
  set_time_value(result, "day", t->d);
  set_time_value(result, "hour", t->h);
  set_time_value(result, "minute", t->i);
  set_time_value(result, "second", t->s);

  if (t->us == TIMELIB_UNSET) {
    result.set_value(string("fraction"), false);
  } else {
    result.set_value(string("fraction"), static_cast<double>(t->us) / 1000000.0);
  }

  result.merge_with(dump_errors(*error));

  result.set_value(string("is_localtime"), static_cast<bool>(t->is_localtime));

  if (t->is_localtime) {
    set_time_value(result, "zone_type", t->zone_type);
    switch (t->zone_type) {
      case TIMELIB_ZONETYPE_OFFSET:
        set_time_value(result, "zone", t->z);
        result.set_value(string("is_dst"), static_cast<bool>(t->dst));
        break;
      case TIMELIB_ZONETYPE_ID:
        if (t->tz_abbr) {
          result.set_value(string("tz_abbr"), string(t->tz_abbr));
        }
        if (t->tz_info) {
          result.set_value(string("tz_id"), string(t->tz_info->name));
        }
        break;
      case TIMELIB_ZONETYPE_ABBR:
        set_time_value(result, "zone", t->z);
        result.set_value(string("is_dst"), static_cast<bool>(t->dst));
        result.set_value(string("tz_abbr"), string(t->tz_abbr));
        break;
    }
  }
  if (t->have_relative) {
    array<mixed> relative;
    relative.set_value(string("year"), t->relative.y);
    relative.set_value(string("month"), t->relative.m);
    relative.set_value(string("day"), t->relative.d);
    relative.set_value(string("hour"), t->relative.h);
    relative.set_value(string("minute"), t->relative.i);
    relative.set_value(string("second"), t->relative.s);
    if (t->relative.have_weekday_relative) {
      relative.set_value(string("weekday"), t->relative.weekday);
    }
    if (t->relative.have_special_relative && (t->relative.special.type == TIMELIB_SPECIAL_WEEKDAY)) {
      relative.set_value(string("weekdays"), t->relative.special.amount);
    }
    if (t->relative.first_last_day_of) {
      string key = string(t->relative.first_last_day_of == TIMELIB_SPECIAL_FIRST_DAY_OF_MONTH ? "first_day_of_month" : "last_day_of_month");
      relative.set_value(key, true);
    }
    result.set_value(string("relative"), relative);
  }

  return result;
}

} // namespace

struct TzinfoCache : private vk::not_copyable {
  friend class vk::singleton<TzinfoCache>;

  timelib_tzinfo *etc_gmt3_;
  timelib_tzinfo *europe_moscow_;

  void init() {
    // if we'll need other timezones, they can be passed as a command-line
    // flag to instruct the runtime to load extra tzinfo objects here

    europe_moscow_ = get_timezone_info(PHP_TIMELIB_TZ_MOSCOW);
    php_assert(europe_moscow_ != nullptr);

    etc_gmt3_ = get_timezone_info(PHP_TIMELIB_TZ_GMT3);
    php_assert(etc_gmt3_ != nullptr);
  }

  timelib_tzinfo *get_tzinfo(const char *tz_name) {
    if (strcmp(PHP_TIMELIB_TZ_MOSCOW, tz_name) == 0) {
      return europe_moscow_;
    }
    if (strcmp(PHP_TIMELIB_TZ_GMT3, tz_name) == 0) {
      return etc_gmt3_;
    }
    return nullptr;
  }

private:
  TzinfoCache() = default;
};

void global_init_php_timelib() {
  vk::singleton<TzinfoCache>::get().init();
}

int php_timelib_days_in_month(int64_t m, int64_t y) {
  return timelib_days_in_month(y, m);
}

bool php_timelib_is_valid_date(int64_t m, int64_t d, int64_t y) {
  return y >= 1 && y <= 32767 && timelib_valid_date(y, m, d);
}

array<mixed> php_timelib_date_parse(const string &time_str) {
  timelib_error_container *error = nullptr;
  timelib_time *t = timelib_strtotime(time_str.c_str(), time_str.size(), &error, timelib_builtin_db(), timelib_parse_tzfile);
  auto t_deleter = vk::finally([t]() { timelib_time_dtor(t); });
  auto error_deleter = vk::finally([error]() { timelib_error_container_dtor(error); });
  return create_date_parse_array(t, error);
}

array<mixed> php_timelib_date_parse_from_format(const string &format, const string &time_str) {
  timelib_error_container *error = nullptr;
  timelib_time *t = timelib_parse_from_format(format.c_str(), time_str.c_str(), time_str.size(), &error, timelib_builtin_db(), timelib_parse_tzfile);
  auto t_deleter = vk::finally([t]() { timelib_time_dtor(t); });
  auto error_deleter = vk::finally([error]() { timelib_error_container_dtor(error); });
  return create_date_parse_array(t, error);
}

std::pair<int64_t, bool> php_timelib_strtotime(const string &tz_name, const string &times, int64_t preset_ts) {
  if (times.empty()) {
    return {0, false};
  }

  timelib_tzinfo *tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(tz_name.c_str());
  if (tzi == nullptr) {
    return {0, false};
  }

  bool use_heap_memory = (dl::get_script_memory_stats().memory_limit == 0);
  auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator(!use_heap_memory);

  timelib_time *now = timelib_time_ctor();
  auto now_deleter = vk::finally([now]() { timelib_time_dtor(now); });

  // can't use a CriticalSectionGuard here: timelib is not prepared for malloc that can return null;
  // but since timelib is thread safe (since 2017), it can be safely interrupted, so we don't need it here

  now->tz_info = tzi;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, static_cast<timelib_sll>(preset_ts));
  timelib_error_container *error = nullptr;
  timelib_time *t = timelib_strtotime(times.c_str(), times.size(), &error, timelib_builtin_db(), timelib_parse_tzfile);
  auto t_deleter = vk::finally([t]() { timelib_time_dtor(t); });

  int errors_num = error->error_count;
  timelib_error_container_dtor(error);
  if (errors_num > 0) {
    return {0, false};
  }

  timelib_fill_holes(t, now, TIMELIB_NO_CLONE);
  timelib_update_ts(t, tzi);
  int conversion_error = 0;
  int64_t ts = timelib_date_to_int(t, &conversion_error);
  if (conversion_error) {
    return {0, false};
  }

  return {ts, true};
}

struct DateGlobals {
  char *default_timezone{nullptr};
  char *timezone{nullptr};
//  HashTable *tzcache{nullptr};
  timelib_error_container *last_errors{nullptr};
  int timezone_valid{0};
};

static DateGlobals date_globals;

static void update_errors_warnings(timelib_error_container *last_errors) {
  if (date_globals.last_errors) {
    timelib_error_container_dtor(date_globals.last_errors);
    date_globals.last_errors = nullptr;
  }
  date_globals.last_errors = last_errors;
}

std::pair<timelib_time *, string> php_timelib_date_initialize(const string &tz_name, const string &time_str, const char *format) {
  timelib_error_container *err = nullptr;
  timelib_time *t = format
    ? timelib_parse_from_format(format, time_str.c_str(), time_str.size(), &err, timelib_builtin_db(), timelib_parse_tzfile)
    : timelib_strtotime(time_str.c_str(), time_str.size(), &err, timelib_builtin_db(), timelib_parse_tzfile);

  /* update last errors and warnings */
  update_errors_warnings(err);

  if (err && err->error_count) {
    /* spit out the first library error message, at least */
    string error_msg{"Failed to parse time string "};
    error_msg.append(1, '(').append(time_str).append(1, ')');
    error_msg.append(" at position ").append(err->error_messages[0].position);
    error_msg.append(" (").append(1, err->error_messages[0].character).append("): ");
    error_msg.append(err->error_messages[0].message);
    timelib_time_dtor(t);
    return {nullptr, std::move(error_msg)};
  }

  timelib_tzinfo *tzi = nullptr;
  if (!tz_name.empty()) {
    tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(tz_name.c_str());
  } else if (t->tz_info) {
    tzi = t->tz_info;
  } else {
    // TODO: use f$date_default_timezone_get()
    tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(PHP_TIMELIB_TZ_MOSCOW);
  }

  timelib_time *now = timelib_time_ctor();
  vk::final_action now_deleter{[now] { timelib_time_dtor(now); }};

  now->tz_info = tzi;
  now->zone_type = TIMELIB_ZONETYPE_ID;

  timeval tp{};
  gettimeofday(&tp, nullptr);
  const auto [sec, usec] = tp;

  timelib_unixtime2local(now, static_cast<timelib_sll>(sec));
  now->us = usec;

  timelib_fill_holes(t, now, TIMELIB_NO_CLONE);
  timelib_update_ts(t, tzi);
  timelib_update_from_sse(t);

  t->have_relative = 0;

  return {t, {}};
}

void php_timelib_date_remove(timelib_time *t) {
  if (t) {
    timelib_time_dtor(t);
  }
}

Optional<array<mixed>> php_timelib_date_get_last_errors() {
  if (date_globals.last_errors) {
    return dump_errors(*date_globals.last_errors);
  }
  return false;
}
