//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt


#pragma once

#include "runtime-core/runtime-core.h"

template<typename T>
T f$_exception_set_location(const T &e, const string &file, int64_t line) {
  php_critical_error("call to unsupported function : _exception_set_location");
}
