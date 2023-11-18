// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_usage.h"

//int64_t f$estimate_memory_usage(const string &value) {
//  if (value.is_reference_counter(ExtraRefCnt::for_global_const) || value.is_reference_counter(ExtraRefCnt::for_instance_cache)) {
//    return 0;
//  }
//  return static_cast<int64_t>(value.estimate_memory_usage());
//}
//
//int64_t f$estimate_memory_usage(const mixed &value) {
//  if (value.is_string()) {
//    return f$estimate_memory_usage(value.as_string());
//  } else if (value.is_array()) {
//    return f$estimate_memory_usage(value.as_array());
//  }
//  return 0;
//}
