// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetime.h"

#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

C$DateTime::~C$DateTime() {
  if (time != nullptr) {
    kphp::timelib::destruct(time);
    time = nullptr;
  }
}

class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime>& self, const string& datetime,
                                                   const class_instance<C$DateTimeZone>& timezone) noexcept {
  const auto& str_to_parse{datetime.empty() ? StringLibConstants::get().NOW_STR : datetime};
  auto expected_time{kphp::timelib::construct_time(std::string_view{str_to_parse.c_str(), str_to_parse.size()})};
  if (!expected_time.has_value()) [[unlikely]] {
    string err_msg;
    format_to(std::back_inserter(err_msg), "DateTime::__construct(): failed to parse datetime ({}): {}", datetime.c_str(), expected_time.error());
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
  }

  timelib_time* time{*expected_time};
  timelib_tzinfo* tzi{!timezone.is_null() ? timezone->tzi : nullptr};
  if (tzi == nullptr) {
    if (time->tz_info != nullptr) {
      tzi = time->tz_info;
    } else if (auto* default_tzi{kphp::timelib::get_timezone_info(TimeInstanceState::get().default_timezone.c_str())}; default_tzi != nullptr) {
      tzi = default_tzi;
    }
  }

  kphp::timelib::fill_holes(time, tzi);

  self->time = time;
  return self;
}
