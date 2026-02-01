// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/date-interval.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval>& self, const string& duration) noexcept {
  auto expected_rel_time{kphp::timelib::parse_interval({duration.c_str(), duration.size()})};
  if (!expected_rel_time.has_value()) [[unlikely]] {
    static constexpr std::string_view AFTER_DURATION{")"};

    string err_msg{"DateInterval::__construct(): "};
    if (expected_rel_time.error()->error_count > 0) {
      static constexpr std::string_view BEFORE_DURATION{"Unknown or bad format ("};
      static constexpr size_t MIN_CAPACITY{BEFORE_DURATION.size() + AFTER_DURATION.size()};

      err_msg.reserve_at_least(MIN_CAPACITY);
      err_msg.append(BEFORE_DURATION.data(), BEFORE_DURATION.size()).append(duration).append(AFTER_DURATION.data(), AFTER_DURATION.size());
    } else {
      static constexpr std::string_view BEFORE_DURATION{"Failed to parse interval ("};
      static constexpr size_t MIN_CAPACITY{BEFORE_DURATION.size() + AFTER_DURATION.size()};

      err_msg.reserve_at_least(MIN_CAPACITY);
      err_msg.append(BEFORE_DURATION.data(), BEFORE_DURATION.size()).append(duration).append(AFTER_DURATION.data(), AFTER_DURATION.size());
    }
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  self->rel_time = std::move(*expected_rel_time);
  return self;
}

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string& datetime) noexcept {
  auto expected{kphp::timelib::parse_time({datetime.c_str(), datetime.size()})};
  if (!expected.has_value()) [[unlikely]] {
    kphp::log::warning("DateInterval::createFromDateString(): Unknown or bad format ({}) {}", datetime.c_str(),
                       kphp::timelib::gen_error_msg(expected.error().get()).c_str());
    return {};
  }
  auto& [time, errors]{*expected};
  auto date_interval{make_instance<C$DateInterval>()};
  date_interval->rel_time = kphp::timelib::clone_time_interval(time);
  return date_interval;
}
