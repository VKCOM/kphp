// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

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

  static MathLibContext& get() noexcept;
};

struct MathLibConstants final : vk::not_copyable {
  string ONE;
  string ZERO;
  string TEN;
  string ZERO_DOT_FIVE;

  bcmath_impl_::BcNum BC_NUM_ONE;
  bcmath_impl_::BcNum BC_NUM_ZERO;
  bcmath_impl_::BcNum BC_ZERO_DOT_FIVE;

  MathLibConstants() noexcept;

  static const MathLibConstants& get() noexcept;
};
