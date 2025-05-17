// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/time/timelib-constants.h"
#include "runtime-light/stdlib/time/timelib-timezone-cache.h"

struct TimeInstanceState final : private vk::not_copyable {
  string default_timezone{kphp::timelib::timezones::MOSCOW.data(), kphp::timelib::timezones::MOSCOW.size()};
  kphp::timelib::timezone_cache timelib_zone_cache;

  TimeInstanceState() noexcept = default;

  static TimeInstanceState& get() noexcept;
};

struct TimeImageState final : private vk::not_copyable {
  kphp::timelib::timezone_cache timelib_zone_cache{kphp::timelib::timezones::MOSCOW, kphp::timelib::timezones::GMT3};

  TimeImageState() noexcept = default;

  static const TimeImageState& get() noexcept;
  static TimeImageState& get_mutable() noexcept;
};
