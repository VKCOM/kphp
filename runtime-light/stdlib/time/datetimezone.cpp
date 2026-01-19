// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetimezone.h"

#include <format>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/iterator.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone>& self, const string& timezone) noexcept {
  if (!timezone.empty()) {
    int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
    self->tzi = kphp::timelib::get_cached_timezone_info(timezone.c_str(), timelib_builtin_db(), std::addressof(errc));
    if (self->tzi == nullptr) [[unlikely]] {
      string err_msg;
      std::format_to(kphp::string_back_insert_iterator{.ref = err_msg}, "DateTimeZone::__construct(): Unknown or bad timezone ({})", timezone.c_str());
      THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    }
  }
  return self;
}

string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone>& self) noexcept {
  return string{self->tzi->name};
}
