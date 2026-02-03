// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-constants.h"
#include "runtime-light/core/reference-counter/reference-counter-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-timezone-cache.h"
#include "runtime-light/stdlib/time/timelib-types.h"

struct TimeImageState final : private vk::not_copyable {
  string NOW_STR{NOW_.data(), static_cast<string::size_type>(NOW_.size())};
  string WARNING_COUNT_STR{WARNING_COUNT_.data(), static_cast<string::size_type>(WARNING_COUNT_.size())};
  string WARNINGS_STR{WARNINGS_.data(), static_cast<string::size_type>(WARNINGS_.size())};
  string ERROR_COUNT_STR{ERROR_COUNT_.data(), static_cast<string::size_type>(ERROR_COUNT_.size())};
  string ERRORS_STR{ERRORS_.data(), static_cast<string::size_type>(ERRORS_.size())};

  kphp::timelib::timezone_cache timelib_zone_cache{kphp::timelib::timezones::MOSCOW, kphp::timelib::timezones::GMT3};

  TimeImageState() noexcept {
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(NOW_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(NOW_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(WARNING_COUNT_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(WARNING_COUNT_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(WARNINGS_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(WARNINGS_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(ERROR_COUNT_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(ERROR_COUNT_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(ERRORS_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(ERRORS_STR, ExtraRefCnt::for_global_const)));
  }

  static const TimeImageState& get() noexcept;
  static TimeImageState& get_mutable() noexcept;

private:
  static constexpr std::string_view NOW_ = "now";
  static constexpr std::string_view WARNING_COUNT_ = "warning_count";
  static constexpr std::string_view WARNINGS_ = "warnings";
  static constexpr std::string_view ERROR_COUNT_ = "error_count";
  static constexpr std::string_view ERRORS_ = "errors";
};

struct TimeInstanceState final : private vk::not_copyable {
  string default_timezone{kphp::timelib::timezones::MOSCOW};
  kphp::timelib::timezone_cache timelib_zone_cache;

  TimeInstanceState() noexcept {
    if (auto errc{k2::set_timezone(kphp::timelib::timezones::MOSCOW)}; errc != k2::errno_ok) [[unlikely]] {
      kphp::log::error("can't set timezone '{}'", kphp::timelib::timezones::MOSCOW);
    }
  }

  std::optional<array<mixed>> get_last_errors() const noexcept {
    if (last_errors == nullptr) {
      return std::nullopt;
    }

    const auto& image_state{TimeImageState::get()};

    array<mixed> result;

    array<string> result_warnings;
    result_warnings.reserve(last_errors->warning_count, false);
    for (size_t i{}; i < last_errors->warning_count; i++) {
      result_warnings.set_value(last_errors->warning_messages[i].position, string{last_errors->warning_messages[i].message});
    }
    result.set_value(image_state.WARNING_COUNT_STR, last_errors->warning_count);
    result.set_value(image_state.WARNINGS_STR, result_warnings);

    array<string> result_errors;
    result_errors.reserve(last_errors->error_count, false);
    for (size_t i{}; i < last_errors->error_count; i++) {
      result_errors.set_value(last_errors->error_messages[i].position, string{last_errors->error_messages[i].message});
    }
    result.set_value(image_state.ERROR_COUNT_STR, last_errors->error_count);
    result.set_value(image_state.ERRORS_STR, result_errors);

    return result;
  }

  void update_last_errors(kphp::timelib::error_container_holder&& new_errors) noexcept {
    last_errors = std::move(new_errors);
  }

  static TimeInstanceState& get() noexcept;

private:
  kphp::timelib::error_container_holder last_errors{nullptr, kphp::timelib::details::error_container_destructor};
};
