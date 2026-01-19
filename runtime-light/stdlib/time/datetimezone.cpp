// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetimezone.h"

#include <cstddef>
#include <format>
#include <functional>
#include <iterator>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"

namespace {

struct string_back_insert_iterator {
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;

  std::reference_wrapper<string> ref;

  string_back_insert_iterator& operator=(char value) noexcept {
    ref.get().push_back(value);
    return *this;
  }

  string_back_insert_iterator& operator*() noexcept {
    return *this;
  }

  string_back_insert_iterator& operator++() noexcept {
    return *this;
  }

  string_back_insert_iterator operator++(int) noexcept { // NOLINT
    return *this;
  }
};

} // namespace

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone>& self, const string& timezone) noexcept {
  if (!timezone.empty()) {
    int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
    self->tzi = kphp::timelib::get_cached_timezone_info(timezone.c_str(), timelib_builtin_db(), std::addressof(errc));
    if (self->tzi == nullptr) [[unlikely]] {
      string err_msg;
      std::format_to(string_back_insert_iterator{.ref = err_msg}, "DateTimeZone::__construct(): Unknown or bad timezone ({})", timezone.c_str());
      THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    }
  }
  return self;
}

string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone>& self) noexcept {
  return string{self->tzi->name};
}
