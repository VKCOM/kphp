// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/dateinterval.h"

#include <format>
#include <iterator>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval>& self, const string& duration) noexcept {
  auto expected_rel_time{kphp::timelib::construct_interval({duration.c_str(), duration.size()})};
  if (!expected_rel_time.has_value()) [[unlikely]] {
    string err_msg{"DateInterval::__construct(): "};
    format_to(std::back_inserter(err_msg), expected_rel_time.error(), duration.c_str());
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  self->rel_time = std::move(*expected_rel_time);
  return self;
}

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string& datetime) noexcept {
  auto [time, errors]{kphp::timelib::construct_time({datetime.c_str(), datetime.size()})};
  if (time == nullptr) [[unlikely]] {
    kphp::log::warning("DateInterval::createFromDateString(): Unknown or bad format ({}) {}", datetime.c_str(), errors);
    return {};
  }
  class_instance<C$DateInterval> date_interval;
  date_interval.alloc();
  date_interval->rel_time = kphp::timelib::clone(time->relative);
  return date_interval;
}

string f$DateInterval$$format(const class_instance<C$DateInterval>& self, const string& format) noexcept {
  string str;
  kphp::timelib::format_to(std::back_inserter(str), {format.c_str(), format.size()}, *self->rel_time);
  return str;
}
