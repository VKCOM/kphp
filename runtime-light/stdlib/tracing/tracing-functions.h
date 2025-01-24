// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/tracing/tracing-event.h"
#include "runtime-light/stdlib/tracing/tracing-span.h"

struct C$KphpDiv;

inline class_instance<C$KphpSpan> f$kphp_tracing_get_current_active_span() noexcept {
  return {};
}

class_instance<C$KphpSpan> f$kphp_tracing_get_root_span() noexcept {
  return {};
}

class_instance<C$KphpDiv> f$kphp_tracing_init([[maybe_unused]] const string &root_span_title) noexcept {
  return {};
}

class_instance<C$KphpSpan> f$kphp_tracing_start_span([[maybe_unused]] const string &title, [[maybe_unused]] const string &short_desc,
                                                     [[maybe_unused]] double start_timestamp) noexcept {
  return {};
}

template<typename F>
void f$kphp_tracing_register_enums_provider([[maybe_unused]] F &&cb_custom_enums) noexcept {}

template<typename F>
void f$kphp_tracing_register_on_finish([[maybe_unused]] F &&cb_should_be_flushed) noexcept {}

template<typename F1, typename F2>
void f$kphp_tracing_register_rpc_details_provider([[maybe_unused]] F1 &&cb_for_typed, [[maybe_unused]] F2 &&cb_for_untyped) noexcept {}
