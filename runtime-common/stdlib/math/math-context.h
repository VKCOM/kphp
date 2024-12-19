// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

namespace math_context_impl_ {

inline constexpr std::string_view ONE = "1";
inline constexpr std::string_view ZERO = "0";
inline constexpr std::string_view TEN = "10";
inline constexpr std::string_view ZERO_DOT_FIVE = "0.5";

} // namespace math_context_impl_

namespace bcmath_impl_ {
struct BcNum {
  int n_sign{0};  // sign
  int n_int{0};   // number of trailing zeroes pluse one if there is '+' or '-' sign
  int n_dot{0};   // number of bytes prior dot
  int n_frac{0};  // n_dot + 1
  int n_scale{0}; // number of digits after dot
  string str;     // actual string representation
};
} // namespace bcmath_impl_

struct MathLibContext final : vk::not_copyable {
  int64_t bc_scale{};

  static MathLibContext &get() noexcept;
};

struct MathLibConstants final : vk::not_copyable {
  string ONE{math_context_impl_::ONE.data(), static_cast<string::size_type>(math_context_impl_::ONE.size())};
  string ZERO{math_context_impl_::ZERO.data(), static_cast<string::size_type>(math_context_impl_::ZERO.size())};
  string TEN{math_context_impl_::TEN.data(), static_cast<string::size_type>(math_context_impl_::TEN.size())};
  string ZERO_DOT_FIVE{math_context_impl_::ZERO_DOT_FIVE.data(), static_cast<string::size_type>(math_context_impl_::ZERO_DOT_FIVE.size())};

  bcmath_impl_::BcNum BC_NUM_ONE;
  bcmath_impl_::BcNum BC_NUM_ZERO;
  bcmath_impl_::BcNum BC_ZERO_DOT_FIVE;

  MathLibConstants() noexcept;

  static const MathLibConstants &get() noexcept;
};
