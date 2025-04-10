// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime/datetime/datetime_interface.h"
#include "runtime/datetime/datetime_zone.h"

struct C$DateInterval;
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

  ~C$DateTime() override;
};

extern const string NOW;
class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime>& self, const string& datetime = NOW,
                                                   const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTime> f$DateTime$$add(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept;

class_instance<C$DateTime> f$DateTime$$createFromFormat(const string& format, const string& datetime,
                                                        const class_instance<C$DateTimeZone>& timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTime> f$DateTime$$createFromImmutable(const class_instance<C$DateTimeImmutable>& object) noexcept;

Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept;

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime>& self, const string& modifier) noexcept;

class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime>& self, int64_t year, int64_t month, int64_t day) noexcept;

class_instance<C$DateTime> f$DateTime$$setISODate(const class_instance<C$DateTime>& self, int64_t year, int64_t week, int64_t dayOfWeek = 1) noexcept;

class_instance<C$DateTime> f$DateTime$$setTime(const class_instance<C$DateTime>& self, int64_t hour, int64_t minute, int64_t second = 0,
                                               int64_t microsecond = 0) noexcept;

class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime>& self, int64_t timestamp) noexcept;

class_instance<C$DateTime> f$DateTime$$sub(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept;

class_instance<C$DateInterval> f$DateTime$$diff(const class_instance<C$DateTime>& self, const class_instance<C$DateTimeInterface>& target_object,
                                                bool absolute = false) noexcept;

string f$DateTime$$format(const class_instance<C$DateTime>& self, const string& format) noexcept;

int64_t f$DateTime$$getOffset(const class_instance<C$DateTime>& self) noexcept;

int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime>& self) noexcept;
