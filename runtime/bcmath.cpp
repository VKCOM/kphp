// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/bcmath.h"
#include "runtime-common/stdlib/math/math-context.h"

void free_bcmath_lib() {
  MathLibContext::get().bc_scale = 0;
}
