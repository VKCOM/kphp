// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime/datetime/timelib_wrapper.h"

struct C$DateInterval : public refcountable_polymorphic_php_classes_virt<>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  timelib_rel_time *rel_time{nullptr};

  virtual const char *get_class() const noexcept {
    return R"(DateInterval)";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$DateInterval::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  ~C$DateInterval() override;
};

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval> &self, const string &duration) noexcept;

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string &datetime) noexcept;

string f$DateInterval$$format(const class_instance<C$DateInterval> &self, const string &format) noexcept;
