// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/tracing/tracing-div.h"
#include "runtime-light/stdlib/tracing/tracing-event.h"
#include "runtime-light/stdlib/tracing/tracing-span.h"

struct C$KphpSpanEvent : public refcountable_php_classes<C$KphpSpanEvent> {
  int span_id{0};

  C$KphpSpanEvent() = default;
  explicit C$KphpSpanEvent(int span_id)
    : span_id(span_id) {}
};

struct C$KphpDiv : public refcountable_php_classes<C$KphpDiv>, private CommonDefaultVisitorMethods {
  using CommonDefaultVisitorMethods::accept;
};

struct C$KphpSpan : public refcountable_php_classes<C$KphpSpan>, private CommonDefaultVisitorMethods {
  using CommonDefaultVisitorMethods::accept;

  int span_id{0};

  C$KphpSpan() = default;
  explicit C$KphpSpan(int span_id)
    : span_id(span_id) {}
};

inline class_instance<C$KphpSpan> f$kphp_tracing_get_current_active_span() noexcept {
  php_warning("called stub kphp_tracing_get_current_active_span");
  return {};
}

inline class_instance<C$KphpSpan> f$kphp_tracing_get_root_span() noexcept {
  php_warning("called stub kphp_tracing_get_root_span");
  return {};
}

inline class_instance<C$KphpDiv> f$kphp_tracing_init([[maybe_unused]] const string &root_span_title) noexcept {
  php_warning("called stub kphp_tracing_init");
  return {};
}

inline class_instance<C$KphpSpan> f$kphp_tracing_start_span([[maybe_unused]] const string &title, [[maybe_unused]] const string &short_desc,
                                                            [[maybe_unused]] double start_timestamp) noexcept {
  php_warning("called stub kphp_tracing_start_span");
  return {};
}

template<typename F>
void f$kphp_tracing_register_enums_provider([[maybe_unused]] F &&cb_custom_enums) noexcept {
  php_warning("called stub kphp_tracing_register_enums_provider");
}

template<typename F>
void f$kphp_tracing_register_on_finish([[maybe_unused]] F &&cb_should_be_flushed) noexcept {
  php_warning("called stub kphp_tracing_register_on_finish");
}

template<typename F1, typename F2>
void f$kphp_tracing_register_rpc_details_provider([[maybe_unused]] F1 &&cb_for_typed, [[maybe_unused]] F2 &&cb_for_untyped) noexcept {
  php_warning("called stub kphp_tracing_register_rpc_details_provider");
}

inline void f$KphpSpan$$addAttributeInt([[maybe_unused]] const class_instance<C$KphpSpan> &v$this, [[maybe_unused]] const string &key,
                                        [[maybe_unused]] int64_t value) noexcept {
  php_warning("called stub KphpSpan::addAttributeInt");
}

inline void f$KphpSpan$$addAttributeString([[maybe_unused]] const class_instance<C$KphpSpan> &v$this, [[maybe_unused]] const string &key,
                                           [[maybe_unused]] const string &value) noexcept {
  php_warning("called stub KphpSpan::addAttributeString");
}

inline class_instance<C$KphpSpanEvent> f$KphpSpan$$addEvent([[maybe_unused]] const class_instance<C$KphpSpan> &v$this, [[maybe_unused]] const string &name,
                                                            [[maybe_unused]] const Optional<double> &manual_timestamp = {}) noexcept {
  php_warning("called stub KphpSpan::addEvent");
  return {};
}

inline void f$KphpSpan$$finish([[maybe_unused]] const class_instance<C$KphpSpan> &v$this,
                               [[maybe_unused]] const Optional<double> &manual_timestamp = {}) noexcept {
  php_warning("called stub KphpSpan::finish");
}

inline void f$KphpSpan$$updateName([[maybe_unused]] const class_instance<C$KphpSpan> &v$this, [[maybe_unused]] const string &title,
                                   [[maybe_unused]] const string &short_desc) noexcept {
  php_warning("called stub KphpSpan::updateName");
}
