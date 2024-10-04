// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/stdlib/system/system-context.h"

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

inline int64_t f$system(const string &command, int64_t &result_code = SystemComponentContext::get().result_code_dummy) {
  php_critical_error("call to unsupported function");
}
