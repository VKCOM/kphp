// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-context.h"
#include "runtime-common/stdlib/math/bcmath-functions.h"

namespace {
inline constexpr std::string_view ONE_ = "1";
inline constexpr std::string_view ZERO_ = "0";
inline constexpr std::string_view TEN_ = "10";
inline constexpr std::string_view ZERO_DOT_FIVE_ = "0.5";
} // namespace

MathLibConstants::MathLibConstants() noexcept
  : ONE(ONE_.data(), static_cast<string::size_type>(ONE_.size()))
  , ZERO(ZERO_.data(), static_cast<string::size_type>(ZERO_.size()))
  , TEN(TEN_.data(), static_cast<string::size_type>(TEN_.size()))
  , ZERO_DOT_FIVE(ZERO_DOT_FIVE_.data(), static_cast<string::size_type>(ZERO_DOT_FIVE_.size()))
  , BC_NUM_ONE(bcmath_impl_::bc_parse_number(ONE).first)
  , BC_NUM_ZERO(bcmath_impl_::bc_parse_number(ZERO).first)
  , BC_ZERO_DOT_FIVE(bcmath_impl_::bc_parse_number(ZERO_DOT_FIVE).first) {

  ONE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ZERO.set_reference_counter_to(ExtraRefCnt::for_global_const);
  TEN.set_reference_counter_to(ExtraRefCnt::for_global_const);
  ZERO_DOT_FIVE.set_reference_counter_to(ExtraRefCnt::for_global_const);

  BC_NUM_ONE.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
  BC_NUM_ZERO.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
  BC_ZERO_DOT_FIVE.str.set_reference_counter_to(ExtraRefCnt::for_global_const);
}
