// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

void f$bcscale(int64_t scale);

string f$bcdiv(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcmod(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcpow(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcadd(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcsub(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcmul(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

int64_t f$bccomp(const string &lhs_str, const string &rhs_str, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcsqrt(const string &num_str, int64_t scale = std::numeric_limits<int64_t>::min());

void free_bcmath_lib();
