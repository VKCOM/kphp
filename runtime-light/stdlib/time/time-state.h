// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-constants.h"
#include "runtime-light/stdlib/time/timelib-functions.h"
#include "runtime-light/stdlib/time/timelib-timezone-cache.h"

struct TimeInstanceState final : private vk::not_copyable {
  string default_timezone{kphp::timelib::timezones::MOSCOW.data(), kphp::timelib::timezones::MOSCOW.size()};
  kphp::timelib::timezone_cache timelib_zone_cache;

  TimeInstanceState() noexcept {
    if (auto errc{k2::set_timezone(kphp::timelib::timezones::MOSCOW)}; errc != k2::errno_ok) [[unlikely]] {
      kphp::log::error("can't set timezone '{}'", kphp::timelib::timezones::MOSCOW);
    }
  }

  Optional<array<mixed>> get_last_errors() const noexcept {
    if (last_errors == nullptr) {
      return false;
    }

    array<mixed> result;

    array<string> result_warnings;
    result_warnings.reserve(last_errors->warning_count, false);
    for (size_t i{}; i < last_errors->warning_count; i++) {
      result_warnings.set_value(last_errors->warning_messages[i].position, string{last_errors->warning_messages[i].message});
    }
    result.set_value(string{"warning_count"}, last_errors->warning_count);
    result.set_value(string{"warnings"}, result_warnings);

    array<string> result_errors;
    result_errors.reserve(last_errors->error_count, false);
    for (size_t i{}; i < last_errors->error_count; i++) {
      result_errors.set_value(last_errors->error_messages[i].position, string{last_errors->error_messages[i].message});
    }
    result.set_value(string{"error_count"}, last_errors->error_count);
    result.set_value(string{"errors"}, result_errors);

    return result;
  }

  void update_last_errors(kphp::timelib::error_container_t&& new_errors) noexcept {
    last_errors.swap(new_errors);
  }

  static TimeInstanceState& get() noexcept;

private:
  kphp::timelib::error_container_t last_errors{nullptr};
};

struct TimeImageState final : private vk::not_copyable {
  kphp::timelib::timezone_cache timelib_zone_cache{kphp::timelib::timezones::MOSCOW, kphp::timelib::timezones::GMT3};

  TimeImageState() noexcept = default;

  static const TimeImageState& get() noexcept;
  static TimeImageState& get_mutable() noexcept;
};
