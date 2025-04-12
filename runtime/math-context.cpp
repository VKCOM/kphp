// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-context.h"

static MathLibContext math_lib_context;

MathLibContext& MathLibContext::get() noexcept {
  return math_lib_context;
}

const static MathLibConstants string_lib_constants;

const MathLibConstants& MathLibConstants::get() noexcept {
  return string_lib_constants;
}
