// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/array/to-array-processor.h"

struct CommonDefaultVisitorMethods {
  // for f$instance_to_array(), f$to_array_debug()
  // set at compiler at deeply_require_to_array_debug_visitor()
  void accept(ToArrayVisitor &) noexcept {}
};
