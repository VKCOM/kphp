// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$DateTimeZone : public refcountable_php_classes<C$DateTimeZone>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  string timezone;

  const char *get_class() const noexcept {
    return R"(DateTimeZone)";
  }

  int get_hash() const noexcept {
    std::string_view name_view{get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
};

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone> &self, const string &timezone) noexcept;
string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone> &self) noexcept;
