// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/tracing/tracing-event.h"

struct C$KphpSpan : public refcountable_php_classes<C$KphpSpan>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  int32_t span_id{0};

  C$KphpSpan() noexcept = default;
  explicit C$KphpSpan(int32_t span_id) noexcept
      : span_id(span_id) {}
};

inline void f$KphpSpan$$addAttributeInt([[maybe_unused]] const class_instance<C$KphpSpan>& v$this, [[maybe_unused]] const string& key,
                                        [[maybe_unused]] int64_t value) noexcept {
  kphp::log::info("called stub KphpSpan::addAttributeInt");
}

inline void f$KphpSpan$$addAttributeString([[maybe_unused]] const class_instance<C$KphpSpan>& v$this, [[maybe_unused]] const string& key,
                                           [[maybe_unused]] const string& value) noexcept {
  kphp::log::info("called stub KphpSpan::addAttributeString");
}

inline class_instance<C$KphpSpanEvent> f$KphpSpan$$addEvent([[maybe_unused]] const class_instance<C$KphpSpan>& v$this, [[maybe_unused]] const string& name,
                                                            [[maybe_unused]] const Optional<double>& manual_timestamp = {}) noexcept {
  kphp::log::info("called stub KphpSpan::addEvent");
  return {};
}

inline void f$KphpSpan$$finish([[maybe_unused]] const class_instance<C$KphpSpan>& v$this,
                               [[maybe_unused]] const Optional<double>& manual_timestamp = {}) noexcept {
  kphp::log::info("called stub KphpSpan::finish");
}

inline void f$KphpSpan$$updateName([[maybe_unused]] const class_instance<C$KphpSpan>& v$this, [[maybe_unused]] const string& title,
                                   [[maybe_unused]] const string& short_desc) noexcept {
  kphp::log::info("called stub KphpSpan::updateName");
}

inline void f$KphpSpan$$finishWithError([[maybe_unused]] const class_instance<C$KphpSpan>& v$this, [[maybe_unused]] int64_t error_code,
                                        [[maybe_unused]] const string& error_msg, [[maybe_unused]] const Optional<float>& manual_timestamp = {}) {
  kphp::log::info("called stub KphpSpan::finishWithError");
}
