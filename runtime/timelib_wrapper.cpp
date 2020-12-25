#include "runtime/timelib_wrapper.h"

#include <kphp/timelib/timelib.h>

#include "common/containers/final_action.h"
#include "common/smart_ptrs/singleton.h"

namespace {
timelib_tzinfo *get_timezone_info(const char *tz_name) {
  int dummy_error_code = 0;
  timelib_tzinfo *info = timelib_parse_tzfile(tz_name, timelib_builtin_db(), &dummy_error_code);
  return info;
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

void php_timelib_init() {
  vk::singleton<TzinfoCache>::get().init();
}

int php_timelib_days_in_month(int64_t m, int64_t y) {
  return timelib_days_in_month(y, m);
}

bool php_timelib_is_valid_date(int64_t m, int64_t d, int64_t y) {
  return y >= 1 && y <= 32767 && timelib_valid_date(y, m, d);
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
