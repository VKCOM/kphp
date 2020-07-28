#pragma once

#include "runtime/kphp_core.h"

void f$bcscale(int64_t scale);

string f$bcdiv(const string &lhs, const string &rhs, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcmod(const string &lhs, const string &rhs);

string f$bcpow(const string &lhs, const string &rhs);

string f$bcadd(const string &lhs, const string &rhs, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcsub(const string &lhs, const string &rhs, int64_t scale = std::numeric_limits<int64_t>::min());

string f$bcmul(const string &lhs, const string &rhs, int64_t scale = std::numeric_limits<int64_t>::min());

int64_t f$bccomp(const string &lhs, const string &rhs, int64_t scale = std::numeric_limits<int64_t>::min());

void free_bcmath_lib();
