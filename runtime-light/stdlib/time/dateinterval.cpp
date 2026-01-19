// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/dateinterval.h"

#include <cstddef>
#include <format>
#include <functional>
#include <iterator>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

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

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval>& self, const string& duration) noexcept {
  auto expected_rel_time{kphp::timelib::parse_interval({duration.c_str(), duration.size()})};
  if (!expected_rel_time.has_value()) [[unlikely]] {
    string err_msg{"DateInterval::__construct(): "};
    if (expected_rel_time.error()->error_count > 0) {
      std::format_to(string_back_insert_iterator{.ref = err_msg}, "Unknown or bad format ({})", duration.c_str());
    } else {
      std::format_to(string_back_insert_iterator{.ref = err_msg}, "Failed to parse interval ({})", duration.c_str());
    }
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  self->rel_time = std::move(*expected_rel_time);
  return self;
}

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string& datetime) noexcept {
  auto [time, errors]{kphp::timelib::parse_time({datetime.c_str(), datetime.size()})};
  if (time == nullptr) [[unlikely]] {
    kphp::log::warning("DateInterval::createFromDateString(): Unknown or bad format ({}) {}", datetime.c_str(), errors);
    return {};
  }
  class_instance<C$DateInterval> date_interval;
  date_interval.alloc();
  date_interval->rel_time = kphp::timelib::clone_time_interval(time);
  return date_interval;
}

string f$DateInterval$$format(const class_instance<C$DateInterval>& self, const string& format) noexcept {
  string str;
  kphp::timelib::format_to(string_back_insert_iterator{.ref = str}, {format.c_str(), format.size()}, self->rel_time);
  return str;
}
