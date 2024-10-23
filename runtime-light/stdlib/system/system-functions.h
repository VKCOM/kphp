// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/runtime-core/utils/kphp-assert-core.h"

template<typename T>
int64_t f$estimate_memory_usage(const T &) {
  php_critical_error("call to unsupported function");
}

template<typename F>
void f$register_shutdown_function(F &&f) {
  php_critical_error("call to unsupported function");
}

template<typename F>
void f$register_kphp_on_warning_callback(F &&callback) {
  php_critical_error("call to unsupported function");
}

template<typename F>
bool f$register_kphp_on_oom_callback(F &&callback) {
  php_critical_error("call to unsupported function");
}

template<typename F>
void f$kphp_extended_instance_cache_metrics_init(F &&callback) {
  php_critical_error("call to unsupported function");
}
