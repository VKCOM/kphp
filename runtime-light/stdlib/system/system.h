// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

template<typename T>
int64_t f$estimate_memory_usage(const T &) {
  php_critical_error("call to unsupported function");
}
