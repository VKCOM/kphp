// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/iterator.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/time/timelib-functions.h"
#include "runtime-light/stdlib/time/timelib-types.h"

struct C$DateTimeZone : public refcountable_polymorphic_php_classes_virt<>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  std::optional<std::reference_wrapper<const kphp::timelib::tzinfo_holder>> tzi;

  virtual const char* get_class() const noexcept {
    return R"(DateTimeZone)";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$DateTimeZone::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  ~C$DateTimeZone() override = default;
};

inline class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone>& self, const string& timezone) noexcept {
  auto expected_tzi{kphp::timelib::get_timezone_info({timezone.c_str(), timezone.size()})};
  if (!expected_tzi.has_value()) [[unlikely]] {
    string err_msg;
    std::format_to(kphp::string_back_insert_iterator{.ref = err_msg}, "DateTimeZone::__construct(): Unknown or bad timezone ({})", timezone.c_str());
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  self->tzi = *expected_tzi;
  return self;
}

inline string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone>& self) noexcept {
  return string{self->tzi->get()->name};
}
