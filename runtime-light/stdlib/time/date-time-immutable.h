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

struct C$DateTime;

struct C$DateTimeImmutable : public C$DateTimeInterface, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  const char* get_class() const noexcept override {
    return R"(DateTimeImmutable)";
  }

  int32_t get_hash() const noexcept override {
    std::string_view name_view{C$DateTimeImmutable::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
};

namespace kphp::time::details {

inline class_instance<C$DateTimeImmutable> clone_immutable(const class_instance<C$DateTimeImmutable>& origin) noexcept {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = kphp::timelib::clone_time(origin->time);
  return clone;
}

} // namespace kphp::time::details

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$__construct(const class_instance<C$DateTimeImmutable>& self,
                                                                     const string& datetime = TimeImageState::get().NOW_STR,
                                                                     const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$add(const class_instance<C$DateTimeImmutable>& self,
                                                                    const class_instance<C$DateInterval>& interval) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  new_date->time = kphp::timelib::add_time_interval(new_date->time, interval->rel_time);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromFormat(const string& format, const string& datetime,
                                                                          const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromMutable(const class_instance<C$DateTime>& object) noexcept;

inline Optional<array<mixed>> f$DateTimeImmutable$$getLastErrors() noexcept {
  auto last_errors{TimeInstanceState::get().get_last_errors()};
  return last_errors.has_value() ? Optional<array<mixed>>{*last_errors} : false;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$modify(const class_instance<C$DateTimeImmutable>& self, const string& modifier) noexcept;

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setDate(const class_instance<C$DateTimeImmutable>& self, int64_t year, int64_t month,
                                                                        int64_t day) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  kphp::timelib::set_date(new_date->time, year, month, day);
  return new_date;
}

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setISODate(const class_instance<C$DateTimeImmutable>& self, int64_t year, int64_t week,
                                                                           int64_t dayOfWeek = 1) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  kphp::timelib::set_isodate(new_date->time, year, week, dayOfWeek);
  return new_date;
}

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTime(const class_instance<C$DateTimeImmutable>& self, int64_t hour, int64_t minute,
                                                                        int64_t second = 0, int64_t microsecond = 0) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  kphp::timelib::set_time(new_date->time, hour, minute, second, microsecond);
  return new_date;
}

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTimestamp(const class_instance<C$DateTimeImmutable>& self, int64_t timestamp) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  kphp::timelib::set_timestamp(new_date->time, timestamp);
  return new_date;
}

inline class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$sub(const class_instance<C$DateTimeImmutable>& self,
                                                                    const class_instance<C$DateInterval>& interval) noexcept {
  auto new_date{kphp::time::details::clone_immutable(self)};
  auto expected_new_time{kphp::timelib::sub_time_interval(new_date->time, interval->rel_time)};
  if (!expected_new_time.has_value()) {
    kphp::log::warning("DateTimeImmutable::sub(): {}", expected_new_time.error());
    return new_date;
  }
  new_date->time = std::move(*expected_new_time);
  return new_date;
}

inline class_instance<C$DateInterval> f$DateTimeImmutable$$diff(const class_instance<C$DateTimeImmutable>& self,
                                                                const class_instance<C$DateTimeInterface>& target_object, bool absolute = false) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = kphp::timelib::get_time_interval(self->time, target_object.get()->time, absolute);
  return interval;
}

inline string f$DateTimeImmutable$$format(const class_instance<C$DateTimeImmutable>& self, const string& format) noexcept {
  if (format.empty()) {
    return {};
  }

  kphp::timelib::time_offset_holder offset{self->time->is_localtime ? kphp::timelib::construct_time_offset(self->time) : nullptr};

  return kphp::timelib::format_time(format, *self->time, offset.get());
}

inline int64_t f$DateTimeImmutable$$getOffset(const class_instance<C$DateTimeImmutable>& self) noexcept {
  return kphp::timelib::get_offset(self->time);
}

inline int64_t f$DateTimeImmutable$$getTimestamp(const class_instance<C$DateTimeImmutable>& self) noexcept {
  return kphp::timelib::get_timestamp(self->time);
}
