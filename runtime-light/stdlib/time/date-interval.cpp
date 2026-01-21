// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/date-interval.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string& datetime) noexcept {
  auto expected{kphp::timelib::parse_time({datetime.c_str(), datetime.size()})};
  if (!expected.has_value()) [[unlikely]] {
    kphp::log::warning("DateInterval::createFromDateString(): Unknown or bad format ({}) {}", datetime.c_str(), expected.error());
    return {};
  }
  auto& [time, errors]{*expected};
  auto date_interval{make_instance<C$DateInterval>()};
  date_interval->rel_time = kphp::timelib::clone_time_interval(time);
  return date_interval;
}
