// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-functions.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/time/date-interval.h"
#include "runtime-light/stdlib/time/date-time-interface.h"
#include "runtime-light/stdlib/time/date-time-zone.h"
#include "runtime-light/stdlib/time/time-state.h"

struct C$DateTimeImmutable;

struct C$DateTime : public C$DateTimeInterface, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  const char* get_class() const noexcept override {
    return R"(DateTime)";
  }

  int32_t get_hash() const noexcept override {
    std::string_view name_view{C$DateTime::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
};

class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime>& self, const string& datetime = TimeImageState::get().NOW_STR,
                                                   const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

inline class_instance<C$DateTime> f$DateTime$$add(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept {
  self->time = kphp::timelib::add_time_interval(self->time, interval->rel_time);
  return self;
}

class_instance<C$DateTime> f$DateTime$$createFromFormat(const string& format, const string& datetime,
                                                        const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTime> f$DateTime$$createFromImmutable(const class_instance<C$DateTimeImmutable>& object) noexcept;

inline Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept {
  auto last_errors{TimeInstanceState::get().get_last_errors()};
  return last_errors.has_value() ? Optional<array<mixed>>{*last_errors} : false;
}

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime>& self, const string& modifier) noexcept;

inline class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime>& self, int64_t year, int64_t month, int64_t day) noexcept {
  kphp::timelib::set_date(self->time, year, month, day);
  return self;
}

inline class_instance<C$DateTime> f$DateTime$$setISODate(const class_instance<C$DateTime>& self, int64_t year, int64_t week, int64_t dayOfWeek = 1) noexcept {
  kphp::timelib::set_isodate(self->time, year, week, dayOfWeek);
  return self;
}

inline class_instance<C$DateTime> f$DateTime$$setTime(const class_instance<C$DateTime>& self, int64_t hour, int64_t minute, int64_t second = 0,
                                                      int64_t microsecond = 0) noexcept {
  kphp::timelib::set_time(self->time, hour, minute, second, microsecond);
  return self;
}

inline class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime>& self, int64_t timestamp) noexcept {
  kphp::timelib::set_timestamp(self->time, timestamp);
  return self;
}

inline class_instance<C$DateTime> f$DateTime$$sub(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept {
  auto expected_new_time{kphp::timelib::sub_time_interval(self->time, interval->rel_time)};
  if (!expected_new_time.has_value()) {
    kphp::log::warning("DateTime::sub(): {}", expected_new_time.error());
    return self;
  }
  self->time = std::move(*expected_new_time);
  return self;
}

inline class_instance<C$DateInterval> f$DateTime$$diff(const class_instance<C$DateTime>& self, const class_instance<C$DateTimeInterface>& target_object,
                                                       bool absolute = false) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = kphp::timelib::get_time_interval(self->time, target_object.get()->time, absolute);
  return interval;
}

inline string f$DateTime$$format(const class_instance<C$DateTime>& self, const string& format) noexcept {
  if (format.empty()) {
    return {};
  }

  kphp::timelib::time_offset_holder offset{self->time->is_localtime ? kphp::timelib::construct_time_offset(self->time) : nullptr};

  return kphp::timelib::format_time(format, *self->time, offset.get());
}

inline int64_t f$DateTime$$getOffset(const class_instance<C$DateTime>& self) noexcept {
  return kphp::timelib::get_offset(self->time);
}

inline int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime>& self) noexcept {
  return kphp::timelib::get_timestamp(self->time);
}
