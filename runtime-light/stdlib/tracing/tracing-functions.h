// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

template <typename F>
void f$kphp_tracing_register_on_finish(F &&cb_should_be_flushed) {
  php_critical_error("call to unsupported function");
}

template <typename F>
void f$kphp_tracing_register_enums_provider(F &&cb_custom_enums) {
  php_critical_error("call to unsupported function");
}

template <typename F1, typename F2>
void f$kphp_tracing_register_rpc_details_provider(F1 &&cb_for_typed, F2 &&cb_for_untyped) {
  php_critical_error("call to unsupported function");
}

