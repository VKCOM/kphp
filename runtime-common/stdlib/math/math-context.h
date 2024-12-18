// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/math/bcmath.h"

namespace math_context_impl {

inline constexpr std::string_view ONE = "1";
inline constexpr std::string_view ZERO = "0";
inline constexpr std::string_view TEN = "10";
inline constexpr std::string_view ZERO_DOT_FIVE = "0.5";

} // namespace math_context_impl

struct MathLibContext final : vk::not_copyable {
  int64_t bc_scale{};

  static MathLibContext &get() noexcept;
};

struct MathLibConstants final : vk::not_copyable {
  string ONE{math_context_impl::ONE.data(), static_cast<string::size_type>(math_context_impl::ONE.size())};
  string ZERO{math_context_impl::ZERO.data(), static_cast<string::size_type>(math_context_impl::ZERO.size())};
  string TEN{math_context_impl::TEN.data(), static_cast<string::size_type>(math_context_impl::TEN.size())};
  string ZERO_DOT_FIVE{math_context_impl::ZERO_DOT_FIVE.data(), static_cast<string::size_type>(math_context_impl::ZERO_DOT_FIVE.size())};

  bcmath_impl_::BcNum BC_NUM_ONE{bcmath_impl_::bc_parse_number(ONE).first};
  bcmath_impl_::BcNum BC_NUM_ZERO{bcmath_impl_::bc_parse_number(ZERO).first};
  bcmath_impl_::BcNum BC_ZERO_DOT_FIVE{bcmath_impl_::bc_parse_number(ZERO_DOT_FIVE).first};

  MathLibConstants() noexcept {
    ONE.set_reference_counter_to(ExtraRefCnt::for_global_const);
    ZERO.set_reference_counter_to(ExtraRefCnt::for_global_const);
    TEN.set_reference_counter_to(ExtraRefCnt::for_global_const);
    ZERO_DOT_FIVE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  }

  static const MathLibConstants &get() noexcept;
};
