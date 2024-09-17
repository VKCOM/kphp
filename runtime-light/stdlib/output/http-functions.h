// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

template<class T>
string f$http_build_query(const array<T> &a, const string &numeric_prefix = {}, const string &arg_separator = string(), int64_t enc_type = 1) {
  php_critical_error("call to unsupported function");
}
