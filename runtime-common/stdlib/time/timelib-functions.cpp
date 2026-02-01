// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-common/stdlib/time/timelib-functions.h"

namespace kphp::timelib {

namespace {

void format_localtime_timezone_identifier_to(string_buffer& sb, timelib_time& t, timelib_time_offset& offset) noexcept {
  switch (t.zone_type) {
  case TIMELIB_ZONETYPE_ID:
    kphp::timelib::details::format_to(sb, "%s", t.tz_info->name);
    break;
  case TIMELIB_ZONETYPE_ABBR:
    kphp::timelib::details::format_to(sb, "%s", offset.abbr);
    break;
  case TIMELIB_ZONETYPE_OFFSET:
    kphp::timelib::details::format_to(sb, "%c%02" PRId32 ":%02" PRId32, kphp::timelib::details::sign(offset.offset), std::abs(offset.offset / 3600),
                                      std::abs((offset.offset % 3600) / 60));
    break;
  }
}

const char* lowercase_ante_meridiem_and_post_meridiem(timelib_sll hours) noexcept {
  return hours >= 12 ? "pm" : "am";
}

const char* uppercase_ante_meridiem_and_post_meridiem(timelib_sll hours) noexcept {
  return hours >= 12 ? "PM" : "AM";
}

timelib_sll hours_12_hour_format(timelib_sll hours) noexcept {
  return (hours % 12) ? hours % 12 : 12;
}

char offset_sign(timelib_time_offset* offset) noexcept {
  return offset != nullptr ? kphp::timelib::details::sign(offset->offset) : '+';
}

int32_t offset_hours(timelib_time_offset* offset) noexcept {
  return offset != nullptr ? std::abs(offset->offset / 3600) : 0;
}

int32_t offset_minutes(timelib_time_offset* offset) noexcept {
  return offset != nullptr ? std::abs((offset->offset % 3600) / 60) : 0;
}

} // namespace

string format_time(const string& format, timelib_time& t, timelib_time_offset* offset) noexcept {
  string_buffer& sb{RuntimeContext::get().static_SB_spare};
  sb.clean();

  timelib_sll isoweek{};
  timelib_sll isoyear{};

  auto ensure_week_year_set{[week_year_set = false, &t, iw = std::addressof(isoweek), iy = std::addressof(isoyear)]() mutable noexcept {
    if (!week_year_set) {
      timelib_isoweek_from_date(t.y, t.m, t.d, iw, iy);
      week_year_set = true;
    }
  }};

  for (size_t i{0}; i < format.size(); ++i) {
    const char* difference_to_gmt_format{"%c%02d%02d"};

    switch (format[i]) {
    // day
    case 'd':
      kphp::timelib::details::format_to(sb, "%02lld", t.d);
      break;
    case 'D':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::details::short_day_name(t.y, t.m, t.d));
      break;
    case 'j':
      kphp::timelib::details::format_to(sb, "%lld", t.d);
      break;
    case 'l':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::details::full_day_name(t.y, t.m, t.d));
      break;
    case 'S':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::details::english_suffix(t.d));
      break;
    case 'w':
      kphp::timelib::details::format_to(sb, "%lld", timelib_day_of_week(t.y, t.m, t.d));
      break;
    case 'N':
      kphp::timelib::details::format_to(sb, "%lld", timelib_iso_day_of_week(t.y, t.m, t.d));
      break;
    case 'z':
      kphp::timelib::details::format_to(sb, "%lld", timelib_day_of_year(t.y, t.m, t.d));
      break;

    // week
    case 'W':
      ensure_week_year_set();
      kphp::timelib::details::format_to(sb, "%02lld", isoweek);
      break; // iso weeknr
    case 'o':
      ensure_week_year_set();
      kphp::timelib::details::format_to(sb, "%lld", isoyear);
      break; // iso year

    // month
    case 'F':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::months::FULL_NAMES[t.m - 1]);
      break;
    case 'm':
      kphp::timelib::details::format_to(sb, "%02lld", t.m);
      break;
    case 'M':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::months::SHORT_NAMES[t.m - 1]);
      break;
    case 'n':
      kphp::timelib::details::format_to(sb, "%lld", t.m);
      break;
    case 't':
      kphp::timelib::details::format_to(sb, "%lld", timelib_days_in_month(t.y, t.m));
      break;

    // year
    case 'L':
      kphp::timelib::details::format_to(sb, "%d", kphp::timelib::is_leap_year(t.y));
      break;
    case 'y':
      kphp::timelib::details::format_to(sb, "%02lld", t.y % 100);
      break;
    case 'Y':
      kphp::timelib::details::format_to(sb, "%s%04lld", kphp::timelib::details::minus_if_negative(t.y), std::abs(t.y));
      break;

    // time
    case 'a':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::lowercase_ante_meridiem_and_post_meridiem(t.h));
      break;
    case 'A':
      kphp::timelib::details::format_to(sb, "%s", kphp::timelib::uppercase_ante_meridiem_and_post_meridiem(t.h));
      break;
    case 'B': {
      auto retval{((t.sse % 86400) + 3600) * 10};
      if (retval < 0) {
        retval += 864000;
      }
      // Make sure to do this on a positive int to avoid rounding errors
      retval = (retval / 864) % 1000;
      kphp::timelib::details::format_to(sb, "%03lld", retval);
      break;
    }
    case 'g':
      kphp::timelib::details::format_to(sb, "%lld", kphp::timelib::hours_12_hour_format(t.h));
      break;
    case 'G':
      kphp::timelib::details::format_to(sb, "%lld", t.h);
      break;
    case 'h':
      kphp::timelib::details::format_to(sb, "%02lld", kphp::timelib::hours_12_hour_format(t.h));
      break;
    case 'H':
      kphp::timelib::details::format_to(sb, "%02lld", t.h);
      break;
    case 'i':
      kphp::timelib::details::format_to(sb, "%02lld", t.i);
      break;
    case 's':
      kphp::timelib::details::format_to(sb, "%02lld", t.s);
      break;
    case 'u':
      kphp::timelib::details::format_to(sb, "%06lld", t.us);
      break;
    case 'v':
      kphp::timelib::details::format_to(sb, "%03lld", t.us / 1000);
      break;

    // timezone
    case 'I':
      kphp::timelib::details::format_to(sb, "%u", offset != nullptr ? offset->is_dst : false);
      break;
    case 'P':
      difference_to_gmt_format = "%c%02d:%02d";
      [[fallthrough]];
    case 'O':
      kphp::timelib::details::format_to(sb, difference_to_gmt_format, kphp::timelib::offset_sign(offset), kphp::timelib::offset_hours(offset),
                                        kphp::timelib::offset_minutes(offset));
      break;
    case 'T':
      kphp::timelib::details::format_to(sb, "%s", offset != nullptr ? offset->abbr : "GMT");
      break;
    case 'e':
      if (offset == nullptr) {
        kphp::timelib::details::format_to(sb, "%s", "UTC");
      } else {
        kphp::timelib::format_localtime_timezone_identifier_to(sb, t, *offset);
      }
      break;
    case 'Z':
      kphp::timelib::details::format_to(sb, "%d", offset != nullptr ? offset->offset : 0);
      break;

    // full date/time
    case 'c':
      kphp::timelib::details::format_to(sb, "%04lld-%02lld-%02lldT%02lld:%02lld:%02lld%c%02d:%02d", t.y, t.m, t.d, t.h, t.i, t.s,
                                        kphp::timelib::offset_sign(offset), kphp::timelib::offset_hours(offset), kphp::timelib::offset_minutes(offset));
      break;
    case 'r':
      kphp::timelib::details::format_to(sb, "%3s, %02lld %3s %04lld %02lld:%02lld:%02lld %c%02d%02d", kphp::timelib::details::short_day_name(t.y, t.m, t.d),
                                        t.d, kphp::timelib::months::SHORT_NAMES[t.m - 1], t.y, t.h, t.i, t.s, kphp::timelib::offset_sign(offset),
                                        kphp::timelib::offset_hours(offset), kphp::timelib::offset_minutes(offset));
      break;
    case 'U':
      kphp::timelib::details::format_to(sb, "%lld", t.sse);
      break;

    case '\\':
      if (i < format.size()) {
        ++i;
      }
      [[fallthrough]];

    default:
      sb << format[i];
      break;
    }
  }

  return sb.str();
}

string gen_error_msg(timelib_error_container* err) noexcept {
  static constexpr std::string_view BEFORE_POSITION{"at position "};
  static constexpr std::string_view BEFORE_CHARACTER{" ("};
  static constexpr std::string_view BEFORE_MESSAGE{"): "};
  static constexpr size_t MIN_CAPACITY{BEFORE_POSITION.size() + BEFORE_CHARACTER.size() + 1 + BEFORE_MESSAGE.size()};

  if (err == nullptr) {
    static constexpr std::string_view UNKNOWN_ERROR{"unknown error"};

    return string{UNKNOWN_ERROR.data(), UNKNOWN_ERROR.size()};
  }

  string error_msg{BEFORE_POSITION.data(), BEFORE_POSITION.size()};
  error_msg.reserve_at_least(MIN_CAPACITY);
  error_msg.append(err->error_messages[0].position);
  error_msg.append(BEFORE_CHARACTER.data(), BEFORE_CHARACTER.size())
      .append(1, err->error_messages[0].character != '\0' ? err->error_messages[0].character : ' ')
      .append(BEFORE_MESSAGE.data(), BEFORE_MESSAGE.size());
  error_msg.append(err->error_messages[0].message);
  return error_msg;
}

} // namespace kphp::timelib
