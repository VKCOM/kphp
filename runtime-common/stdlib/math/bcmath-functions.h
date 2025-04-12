// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/math/math-context.h"

namespace bcmath_impl_ {

std::pair<BcNum, bool> bc_parse_number(const string &num) noexcept;

} // namespace bcmath_impl_

void f$bcscale(int64_t scale) noexcept;

string f$bcdiv(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcmod(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcpow(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcadd(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcsub(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcmul(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

int64_t f$bccomp(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;

string f$bcsqrt(const string &num_str, int64_t scale = std::numeric_limits<int64_t>::min()) noexcept;
