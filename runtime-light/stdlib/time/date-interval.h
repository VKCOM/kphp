// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-functions.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/time/timelib-functions.h"
#include "runtime-light/stdlib/time/timelib-types.h"

struct C$DateInterval : public refcountable_polymorphic_php_classes_virt<>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  kphp::timelib::rel_time_holder rel_time{nullptr};

  virtual const char* get_class() const noexcept {
    return R"(DateInterval)";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$DateInterval::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
};

inline class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval>& self, const string& duration) noexcept {
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

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string& datetime) noexcept;

inline string f$DateInterval$$format(const class_instance<C$DateInterval>& self, const string& format) noexcept {
  return kphp::timelib::format_rel_time(format, *self->rel_time);
}
