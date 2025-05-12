// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/tracing/tracing-div.h"
#include "runtime-light/stdlib/tracing/tracing-span.h"
#include "runtime-light/utils/logs.h"

inline class_instance<C$KphpSpan> f$kphp_tracing_get_current_active_span() noexcept {
  kphp::log::warning("called stub kphp_tracing_get_current_active_span");
  return {};
}

inline void f$kphp_tracing_func_enter_branch(int /*$branch_num*/) noexcept {
  kphp::log::warning("called stub kphp_tracing_func_enter_branch");
}

inline class_instance<C$KphpSpan> f$kphp_tracing_get_root_span() noexcept {
  kphp::log::warning("called stub kphp_tracing_get_root_span");
  return {};
}

inline class_instance<C$KphpDiv> f$kphp_tracing_init([[maybe_unused]] const string& root_span_title) noexcept {
  kphp::log::warning("called stub kphp_tracing_init");
  return {};
}

inline class_instance<C$KphpSpan> f$kphp_tracing_start_span([[maybe_unused]] const string& title, [[maybe_unused]] const string& short_desc,
                                                            [[maybe_unused]] double start_timestamp) noexcept {
  kphp::log::warning("called stub kphp_tracing_start_span");
  return {};
}

template<typename F>
void f$kphp_tracing_register_enums_provider([[maybe_unused]] F&& cb_custom_enums) noexcept {
  kphp::log::warning("called stub kphp_tracing_register_enums_provider");
}

template<typename F>
void f$kphp_tracing_register_on_finish([[maybe_unused]] F&& cb_should_be_flushed) noexcept {
  kphp::log::warning("called stub kphp_tracing_register_on_finish");
}

template<typename F1, typename F2>
void f$kphp_tracing_register_rpc_details_provider([[maybe_unused]] F1&& cb_for_typed, [[maybe_unused]] F2&& cb_for_untyped) noexcept {
  kphp::log::warning("called stub kphp_tracing_register_rpc_details_provider");
}

inline void f$kphp_tracing_set_level([[maybe_unused]] int64_t trace_level) noexcept {
  kphp::log::warning("called stub kphp_tracing_set_level");
}
