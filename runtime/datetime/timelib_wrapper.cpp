#include "runtime/datetime/timelib_wrapper.h"

#include <string_view>

#include "kphp/timelib/timelib.h"
#include "runtime-common/stdlib/time/timelib-functions.h"
#if ASAN_ENABLED
#include <sanitizer/lsan_interface.h>
#endif
#include <sys/time.h>

#include "common/containers/final_action.h"
#include "common/smart_ptrs/singleton.h"
#include "runtime-common/stdlib/time/timelib-constants.h"
#include "runtime/allocator.h"
#include "server/php-runner.h"

// these constants are a part of the private timelib API, but PHP uses them internally;
// we define them here locally
constexpr int TIMELIB_SPECIAL_WEEKDAY = 0x01;
constexpr int TIMELIB_SPECIAL_FIRST_DAY_OF_MONTH = 0x01;

namespace {

timelib_tzinfo* get_timezone_info(const char* tz_name) {
  int dummy_error_code = 0;
  timelib_tzinfo* info = timelib_parse_tzfile(tz_name, timelib_builtin_db(), &dummy_error_code);
  return info;
}

void set_time_value(array<mixed>& dst, const char* name, int64_t value) {
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

array<mixed> dump_errors(const timelib_error_container& error) {
  static constexpr std::string_view WARNING_COUNT{"warning_count"};
  static constexpr std::string_view WARNINGS{"warnings"};
  static constexpr std::string_view ERROR_COUNT{"error_count"};
  static constexpr std::string_view ERRORS{"errors"};

  array<mixed> result;

  array<string> result_warnings;
  result_warnings.reserve(error.warning_count, false);
  for (int i = 0; i < error.warning_count; i++) {
    result_warnings.set_value(error.warning_messages[i].position, string(error.warning_messages[i].message));
  }
  result.set_value(string(WARNING_COUNT.data(), WARNING_COUNT.size()), error.warning_count);
  result.set_value(string(WARNINGS.data(), WARNINGS.size()), result_warnings);

  array<string> result_errors;
  result_errors.reserve(error.error_count, false);
  for (int i = 0; i < error.error_count; i++) {
    result_errors.set_value(error.error_messages[i].position, string(error.error_messages[i].message));
  }
  result.set_value(string(ERROR_COUNT.data(), ERROR_COUNT.size()), error.error_count);
  result.set_value(string(ERRORS.data(), ERRORS.size()), result_errors);

  return result;
}

array<mixed> create_date_parse_array(timelib_time* t, timelib_error_container* error) {
  static constexpr std::string_view FRACTION{"fraction"};
  static constexpr std::string_view IS_LOCALTIME{"is_localtime"};

  array<mixed> result;

  // note: we're setting the result array keys in the same order as PHP does

  set_time_value(result, "year", t->y);
  set_time_value(result, "month", t->m);
  set_time_value(result, "day", t->d);
  set_time_value(result, "hour", t->h);
  set_time_value(result, "minute", t->i);
  set_time_value(result, "second", t->s);

  if (t->us == TIMELIB_UNSET) {
    result.set_value(string(FRACTION.data(), FRACTION.size()), false);
  } else {
    result.set_value(string(FRACTION.data(), FRACTION.size()), static_cast<double>(t->us) / 1000000.0);
  }

  result.merge_with(dump_errors(*error));

  result.set_value(string(IS_LOCALTIME.data(), IS_LOCALTIME.size()), static_cast<bool>(t->is_localtime));

  if (t->is_localtime) {
    static constexpr std::string_view IS_DST{"is_dst"};
    static constexpr std::string_view TZ_ABBR{"tz_abbr"};
    static constexpr std::string_view TZ_ID{"tz_id"};

    set_time_value(result, "zone_type", t->zone_type);
    switch (t->zone_type) {
    case TIMELIB_ZONETYPE_OFFSET:
      set_time_value(result, "zone", t->z);
      result.set_value(string(IS_DST.data(), IS_DST.size()), static_cast<bool>(t->dst));
      break;
    case TIMELIB_ZONETYPE_ID:
      if (t->tz_abbr) {
        result.set_value(string(TZ_ABBR.data(), TZ_ABBR.size()), string(t->tz_abbr));
      }
      if (t->tz_info) {
        result.set_value(string(TZ_ID.data(), TZ_ID.size()), string(t->tz_info->name));
      }
      break;
    case TIMELIB_ZONETYPE_ABBR:
      set_time_value(result, "zone", t->z);
      result.set_value(string(IS_DST.data(), IS_DST.size()), static_cast<bool>(t->dst));
      result.set_value(string(TZ_ABBR.data(), TZ_ABBR.size()), string(t->tz_abbr));
      break;
    }
  }
  if (t->have_relative) {
    static constexpr std::string_view YEAR{"year"};
    static constexpr std::string_view MONTH{"month"};
    static constexpr std::string_view DAY{"day"};
    static constexpr std::string_view HOUR{"hour"};
    static constexpr std::string_view MINUTE{"minute"};
    static constexpr std::string_view SECOND{"second"};
    static constexpr std::string_view RELATIVE{"relative"};

    array<mixed> relative;
    relative.set_value(string(YEAR.data(), YEAR.size()), t->relative.y);
    relative.set_value(string(MONTH.data(), MONTH.size()), t->relative.m);
    relative.set_value(string(DAY.data(), DAY.size()), t->relative.d);
    relative.set_value(string(HOUR.data(), HOUR.size()), t->relative.h);
    relative.set_value(string(MINUTE.data(), MINUTE.size()), t->relative.i);
    relative.set_value(string(SECOND.data(), SECOND.size()), t->relative.s);
    if (t->relative.have_weekday_relative) {
      static constexpr std::string_view WEEKDAY{"weekday"};

      relative.set_value(string(WEEKDAY.data(), WEEKDAY.size()), t->relative.weekday);
    }
    if (t->relative.have_special_relative && (t->relative.special.type == TIMELIB_SPECIAL_WEEKDAY)) {
      static constexpr std::string_view WEEKDAYS{"weekdays"};

      relative.set_value(string(WEEKDAYS.data(), WEEKDAYS.size()), t->relative.special.amount);
    }
    if (t->relative.first_last_day_of) {
      string key = string(t->relative.first_last_day_of == TIMELIB_SPECIAL_FIRST_DAY_OF_MONTH ? "first_day_of_month" : "last_day_of_month");
      relative.set_value(key, true);
    }
    result.set_value(string(RELATIVE.data(), RELATIVE.size()), relative);
  }

  return result;
}

} // namespace

struct TzinfoCache : private vk::not_copyable {
  friend class vk::singleton<TzinfoCache>;

  timelib_tzinfo* etc_gmt3_;
  timelib_tzinfo* europe_moscow_;

  void init() {
    // if we'll need other timezones, they can be passed as a command-line
    // flag to instruct the runtime to load extra tzinfo objects here

    europe_moscow_ = get_timezone_info(kphp::timelib::timezones::MOSCOW);
    php_assert(europe_moscow_ != nullptr);

    etc_gmt3_ = get_timezone_info(kphp::timelib::timezones::GMT3);
    php_assert(etc_gmt3_ != nullptr);
  }

  timelib_tzinfo* get_tzinfo(const char* tz_name) {
    if (strcmp(kphp::timelib::timezones::MOSCOW, tz_name) == 0) {
      return europe_moscow_;
    }
    if (strcmp(kphp::timelib::timezones::GMT3, tz_name) == 0) {
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

static timelib_time* timelib_strtotime_leak_safe(const string& time, timelib_error_container** errors) {
  timelib_time* t = timelib_strtotime(time.c_str(), time.size(), errors, timelib_builtin_db(), timelib_parse_tzfile);
  // sometimes 't->tz_info' contains allocated timezone object, but sometimes doesn't. And we are unable to merely delete this object
  // because it can be shared among different e.g. DateTimeImmutable vs DateTime objects. Given that it's quite rare situation this leak is just suppressed.
  // (it's not a leak in 'php-src' because all 'tz_info *' there are cached. We are unable cache it in kphp because of our memory managment)
#if ASAN_ENABLED
  __lsan_ignore_object(t->tz_info);
#endif
  return t;
}

static timelib_time* timelib_parse_from_format_leak_safe(const char* format, const string& time, timelib_error_container** errors) {
  timelib_time* t = timelib_parse_from_format(format, time.c_str(), time.size(), errors, timelib_builtin_db(), timelib_parse_tzfile);
#if ASAN_ENABLED
  __lsan_ignore_object(t->tz_info);
#endif
  return t;
}

array<mixed> php_timelib_date_parse(const string& time_str) {
  timelib_error_container* error = nullptr;
  timelib_time* t = timelib_strtotime_leak_safe(time_str, &error);
  auto t_deleter = vk::finally([t]() { timelib_time_dtor(t); });
  auto error_deleter = vk::finally([error]() { timelib_error_container_dtor(error); });
  return create_date_parse_array(t, error);
}

array<mixed> php_timelib_date_parse_from_format(const string& format, const string& time_str) {
  timelib_error_container* error = nullptr;
  timelib_time* t = timelib_parse_from_format_leak_safe(format.c_str(), time_str, &error);
  auto t_deleter = vk::finally([t]() { timelib_time_dtor(t); });
  auto error_deleter = vk::finally([error]() { timelib_error_container_dtor(error); });
  return create_date_parse_array(t, error);
}

std::pair<int64_t, bool> php_timelib_strtotime(const string& tz_name, const string& times, int64_t preset_ts) {
  if (times.empty()) {
    return {0, false};
  }

  timelib_tzinfo* tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(tz_name.c_str());
  if (tzi == nullptr) {
    return {0, false};
  }

  bool use_heap_memory = !(php_script.has_value() && php_script->is_running());
  auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator(!use_heap_memory);

  timelib_time* now = timelib_time_ctor();
  auto now_deleter = vk::finally([now]() { timelib_time_dtor(now); });

  // can't use a CriticalSectionGuard here: timelib is not prepared for malloc that can return null;
  // but since timelib is thread safe (since 2017), it can be safely interrupted, so we don't need it here

  now->tz_info = tzi;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, static_cast<timelib_sll>(preset_ts));
  timelib_error_container* error = nullptr;
  timelib_time* t = timelib_strtotime_leak_safe(times, &error);
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

static timelib_error_container* last_errors_global = nullptr;

// NB: should be called under script allocator, because of calls to free() inside timelib_error_container_dtor()
static void update_errors_warnings(timelib_error_container* last_errors, [[maybe_unused]] const ScriptMemGuard& guard) {
  if (last_errors_global) {
    timelib_error_container_dtor(last_errors_global);
    last_errors_global = nullptr;
  }
  last_errors_global = last_errors;
}

void free_timelib() {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  update_errors_warnings(nullptr, script_guard);
}

static const string NOW{"now"};

std::pair<timelib_time*, string> php_timelib_date_initialize(const string& tz_name, const string& time_str, const char* format) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  const string& time_str_new = time_str.empty() ? NOW : time_str;
  timelib_error_container* err = nullptr;
  timelib_time* t = format ? timelib_parse_from_format_leak_safe(format, time_str, &err) : timelib_strtotime_leak_safe(time_str_new, &err);

  // update last errors and warnings
  update_errors_warnings(err, script_guard);

  if (err && err->error_count) {
    timelib_time_dtor(t);

    // spit out the first library error message, at least
    static constexpr std::string_view MESSAGE_PREFIX{"Failed to parse time string "};

    string err_msg{MESSAGE_PREFIX.data(), MESSAGE_PREFIX.size()};
    err_msg.reserve_at_least(MESSAGE_PREFIX.size() + 1 + time_str.size() + 2);
    return {nullptr, err_msg.append(1, '(').append(time_str).append(1, ')').append(1, ' ').append(kphp::timelib::gen_error_msg(err))};
  }

  timelib_tzinfo* tzi = nullptr;
  if (!tz_name.empty()) {
    tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(tz_name.c_str());
  } else if (t->tz_info) {
    tzi = t->tz_info;
  } else {
    // TODO: use f$date_default_timezone_get()
    tzi = vk::singleton<TzinfoCache>::get().get_tzinfo(kphp::timelib::timezones::MOSCOW);
  }

  timelib_time* now = timelib_time_ctor();
  vk::final_action now_deleter{[now] { timelib_time_dtor(now); }};

  now->tz_info = tzi;
  now->zone_type = TIMELIB_ZONETYPE_ID;

  timeval tp{};
  gettimeofday(&tp, nullptr);
  const auto [sec, usec] = tp;

  timelib_unixtime2local(now, static_cast<timelib_sll>(sec));
  now->us = usec;

  int options = TIMELIB_NO_CLONE;
  if (format) {
    options |= TIMELIB_OVERRIDE_TIME;
  }

  timelib_fill_holes(t, now, options);
  timelib_update_ts(t, tzi);
  timelib_update_from_sse(t);

  t->have_relative = 0;

  return {t, {}};
}

timelib_time* php_timelib_time_clone(timelib_time* t) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  return timelib_time_clone(t);
}

void php_timelib_date_remove(timelib_time* t) {
  if (t) {
    auto script_guard = make_malloc_replacement_with_script_allocator();
    timelib_time_dtor(t);
  }
}

Optional<array<mixed>> php_timelib_date_get_last_errors() {
  if (last_errors_global) {
    return dump_errors(*last_errors_global);
  }
  return false;
}

void php_timelib_date_timestamp_set(timelib_time* t, int64_t timestamp) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  timelib_unixtime2local(t, static_cast<timelib_sll>(timestamp));
  timelib_update_ts(t, nullptr);
  t->us = 0;
}

int64_t php_timelib_date_timestamp_get(timelib_time* t) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_update_ts(t, nullptr);

  int error = 0;
  timelib_long timestamp = timelib_date_to_int(t, &error);
  // the 'error' should be always 0 on x64 platform
  php_assert(error == 0);

  return timestamp;
}

std::pair<bool, string> php_timelib_date_modify(timelib_time* t, const string& modifier) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_error_container* err = nullptr;
  timelib_time* tmp_time = timelib_strtotime_leak_safe(modifier, &err);
  vk::final_action tmp_time_deleter{[tmp_time] { timelib_time_dtor(tmp_time); }};

  // update last errors and warnings
  update_errors_warnings(err, script_guard);

  if (err && err->error_count) {
    // spit out the first library error message, at least
    static constexpr std::string_view MESSAGE_PREFIX{"Failed to parse time string "};

    string err_msg{MESSAGE_PREFIX.data(), MESSAGE_PREFIX.size()};
    err_msg.reserve_at_least(MESSAGE_PREFIX.size() + 1 + modifier.size() + 2);
    return {false, err_msg.append(1, '(').append(modifier).append(1, ')').append(1, ' ').append(kphp::timelib::gen_error_msg(err))};
  }

  std::memcpy(&t->relative, &tmp_time->relative, sizeof(timelib_rel_time));
  t->have_relative = tmp_time->have_relative;
  t->sse_uptodate = 0;

  if (tmp_time->y != TIMELIB_UNSET) {
    t->y = tmp_time->y;
  }
  if (tmp_time->m != TIMELIB_UNSET) {
    t->m = tmp_time->m;
  }
  if (tmp_time->d != TIMELIB_UNSET) {
    t->d = tmp_time->d;
  }

  if (tmp_time->h != TIMELIB_UNSET) {
    t->h = tmp_time->h;
    if (tmp_time->i != TIMELIB_UNSET) {
      t->i = tmp_time->i;
      if (tmp_time->s != TIMELIB_UNSET) {
        t->s = tmp_time->s;
      } else {
        t->s = 0;
      }
    } else {
      t->i = 0;
      t->s = 0;
    }
  }

  if (tmp_time->us != TIMELIB_UNSET) {
    t->us = tmp_time->us;
  }

  timelib_update_ts(t, nullptr);
  timelib_update_from_sse(t);
  t->have_relative = 0;
  std::memset(&t->relative, 0, sizeof(t->relative));

  return {true, {}};
}

void php_timelib_date_date_set(timelib_time* t, int64_t y, int64_t m, int64_t d) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  t->y = y;
  t->m = m;
  t->d = d;
  timelib_update_ts(t, nullptr);
}

void php_timelib_date_isodate_set(timelib_time* t, int64_t y, int64_t w, int64_t d) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  t->y = y;
  t->m = 1;
  t->d = 1;
  std::memset(&t->relative, 0, sizeof(t->relative));
  t->relative.d = timelib_daynr_from_weeknr(y, w, d);
  t->have_relative = 1;
  timelib_update_ts(t, nullptr);
}

void php_date_time_set(timelib_time* t, int64_t h, int64_t i, int64_t s, int64_t ms) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  t->h = h;
  t->i = i;
  t->s = s;
  t->us = ms;
  timelib_update_ts(t, nullptr);
}

int64_t php_timelib_date_offset_get(timelib_time* t) {
  auto script_guard = make_malloc_replacement_with_script_allocator();
  if (t->is_localtime) {
    switch (t->zone_type) {
    case TIMELIB_ZONETYPE_ID: {
      timelib_time_offset* offset = timelib_get_time_zone_info(t->sse, t->tz_info);
      int64_t offset_int = offset->offset;
      timelib_time_offset_dtor(offset);
      return offset_int;
    }
    case TIMELIB_ZONETYPE_OFFSET: {
      return t->z;
    }
    case TIMELIB_ZONETYPE_ABBR: {
      return t->z + (3600 * t->dst);
    }
    }
  }
  return 0;
}

timelib_time* php_timelib_date_add(timelib_time* t, timelib_rel_time* interval) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_time* new_time = timelib_add(t, interval);
  timelib_time_dtor(t);
  return new_time;
}

std::pair<timelib_time*, std::string_view> php_timelib_date_sub(timelib_time* t, timelib_rel_time* interval) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  if (interval->have_special_relative) {
    return {nullptr, "Only non-special relative time specifications are supported for subtraction"};
  }

  timelib_time* new_time = timelib_sub(t, interval);
  timelib_time_dtor(t);
  return {new_time, {}};
}

timelib_rel_time* php_timelib_date_diff(timelib_time* time1, timelib_time* time2, bool absolute) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_update_ts(time1, nullptr);
  timelib_update_ts(time2, nullptr);

  timelib_rel_time* diff = timelib_diff(time1, time2);
  if (absolute) {
    diff->invert = 0;
  }
  return diff;
}

std::pair<timelib_rel_time*, string> php_timelib_date_interval_initialize(const string& format) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_time* b = nullptr;
  timelib_time* e = nullptr;
  timelib_rel_time* p = nullptr;
  int r = 0;
  timelib_error_container* errors = nullptr;
  timelib_strtointerval(format.c_str(), format.size(), &b, &e, &p, &r, &errors);
  vk::final_action error_deleter{[errors]() { timelib_error_container_dtor(errors); }};
  vk::final_action e_deleter{[e]() { free(e); }};
  vk::final_action b_deleter{[b]() { free(b); }};

  if (errors->error_count > 0) {
    if (p) {
      timelib_rel_time_dtor(p);
    }

    static constexpr std::string_view MESSAGE_PREFIX{"Unknown or bad format ("};

    string err_msg{MESSAGE_PREFIX.data(), MESSAGE_PREFIX.size()};
    err_msg.reserve_at_least(MESSAGE_PREFIX.size() + format.size() + 1);
    return {nullptr, err_msg.append(format).append(1, ')')};
  }

  if (p) {
    return {p, {}};
  }

  if (b && e) {
    timelib_update_ts(b, nullptr);
    timelib_update_ts(e, nullptr);
    return {timelib_diff(b, e), {}};
  }

  static constexpr std::string_view MESSAGE_PREFIX{"Failed to parse interval ("};

  string err_msg{MESSAGE_PREFIX.data(), MESSAGE_PREFIX.size()};
  err_msg.reserve_at_least(MESSAGE_PREFIX.size() + format.size() + 1);

  return {nullptr, err_msg.append(format).append(1, ')')};
}

void php_timelib_date_interval_remove(timelib_rel_time* t) {
  if (t) {
    auto script_guard = make_malloc_replacement_with_script_allocator();
    timelib_rel_time_dtor(t);
  }
}

std::pair<timelib_rel_time*, string> php_timelib_date_interval_create_from_date_string(const string& time_str) {
  auto script_guard = make_malloc_replacement_with_script_allocator();

  timelib_error_container* err = nullptr;
  timelib_time* time = timelib_strtotime_leak_safe(time_str, &err);
  vk::final_action time_deleter{[time] { timelib_time_dtor(time); }};
  vk::final_action error_deleter{[err]() { timelib_error_container_dtor(err); }};

  if (err->error_count > 0) {
    static constexpr std::string_view MESSAGE_PREFIX{"Unknown or bad format ("};

    string error_msg{MESSAGE_PREFIX.data(), MESSAGE_PREFIX.size()};
    error_msg.reserve_at_least(MESSAGE_PREFIX.size() + time_str.size() + 2);
    error_msg.append(time_str).append(1, ')').append(1, ' ').append(kphp::timelib::gen_error_msg(err));
    return {nullptr, std::move(error_msg)};
  }
  return {timelib_rel_time_clone(&time->relative), {}};
}
