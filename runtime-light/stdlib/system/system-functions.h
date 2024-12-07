// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/stdlib/system/system-state.h"

template<typename T>
int64_t f$estimate_memory_usage(const T &) {
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

inline int64_t f$system(const string &command, int64_t &result_code = SystemInstanceState::get().result_code_dummy) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$getopt(const string &options, array<string> longopts = {},
                                       Optional<int64_t> &rest_index = SystemInstanceState::get().rest_index_dummy) {
  php_critical_error("call to unsupported function");
}
