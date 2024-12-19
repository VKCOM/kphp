// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-context.h"
#include "runtime-common/stdlib/math/bcmath-functions.h"

MathLibConstants::MathLibConstants() noexcept {
  ONE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ZERO.set_reference_counter_to(ExtraRefCnt::for_global_const);
  TEN.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ZERO_DOT_FIVE.set_reference_counter_to(ExtraRefCnt::for_global_const);

  BC_NUM_ONE = bcmath_impl_::bc_parse_number(ONE).first;
  BC_NUM_ZERO = bcmath_impl_::bc_parse_number(ZERO).first;
  BC_ZERO_DOT_FIVE = bcmath_impl_::bc_parse_number(ZERO_DOT_FIVE).first;

  BC_NUM_ONE.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
  BC_NUM_ZERO.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
  BC_ZERO_DOT_FIVE.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
}
