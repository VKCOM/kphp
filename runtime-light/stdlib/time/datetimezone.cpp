// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetimezone.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone>& self, const string& timezone) noexcept {
  if (!timezone.empty()) {
    int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
    self->tzi = kphp::timelib::get_timezone_info(timezone.c_str(), timelib_builtin_db(), std::addressof(errc));
    if (self->tzi == nullptr) [[unlikely]] {
      string err_msg;
      format_to(std::back_inserter(err_msg), "DateTimeZone::__construct(): can't get timezone info: timezone -> {}, error -> {}", timezone.c_str(),
                timelib_get_error_message(errc));
      THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    }
  }
  return self;
}

string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone>& self) noexcept {
  return string{self->tzi->name};
}
