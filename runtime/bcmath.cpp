// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/bcmath.h"

namespace {

int bc_scale{0};

const string ONE("1", 1);
const string ZERO("0", 1);

string bc_zero(int scale) {
  if (scale == 0) {
    return ZERO;
  }
  string result(scale + 2, '0');
  result[1] = '.';
  return result;
}

// legacy code is isolated within this namespace;
// it would be nice to gradually rewrite it with some kind of OOP-style
namespace legacy {

// parse a number into parts, returns scale on success and -1 on error
int bc_parse_number_impl(const string &s, int &lsign, int &lint, int &ldot, int &lfrac, int &lscale) {
  int i = 0;
  lsign = 1;
  if (s[i] == '-' || s[i] == '+') {
    if (s[i] == '-') {
      lsign = -1;
    }
    i++;
  }
  if (!s[i]) {
    return -1;
  }
  lint = i;

  while ('0' <= s[i] && s[i] <= '9') {
    i++;
  }
  ldot = i;

  lscale = 0;
  if (s[i] == '.') {
    lscale = (int)s.size() - i - 1;
    i++;
  }
  lfrac = i;

  while ('0' <= s[i] && s[i] <= '9') {
    i++;
  }

  if (s[i]) {
    return -1;
  }

  while (s[lint] == '0' && lint + 1 < ldot) {
    lint++;
  }

  if (lscale == 0 && lfrac > ldot) {
    lfrac--;
    php_assert(lfrac == ldot);
  }

  if (lsign < 0 && (lscale == 0 && s[lint] == '0')) {
    lsign = 1;
  }
  return lscale;
}

int bc_comp_impl(const char *lhs, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rint, int rdot, int rfrac, int rscale, int scale) {
  int llen = ldot - lint;
  int rlen = rdot - rint;

  if (llen != rlen) {
    return (llen < rlen ? -1 : 1);
  }

  for (int i = 0; i < llen; i++) {
    if (lhs[lint + i] != rhs[rint + i]) {
      return (lhs[lint + i] < rhs[rint + i] ? -1 : 1);
    }
  }

  int i;
  for (i = 0; (i < lscale || i < rscale) && i < scale; i++) {
    int lchar = (i < lscale ? lhs[lfrac + i] : '0');
    int rchar = (i < rscale ? rhs[rfrac + i] : '0');
    if (lchar != rchar) {
      return (lchar < rchar ? -1 : 1);
    }
  }

  return 0;
}

string bc_round(char *lhs, int lint, int ldot, int lfrac, int lscale, int scale, int sign, int add_trailing_zeroes) {
  while (lhs[lint] == '0' && lint + 1 < ldot) {
    lint++;
  }

  php_assert(lint > 0 && lscale >= 0 && scale >= 0);

  if (lscale > scale) {
    lscale = scale;
  }

  if (sign < 0 && lhs[lint] == '0') {
    sign = 1;
    for (int i = 0; i < lscale; i++) {
      if (lhs[lfrac + i] != '0') {
        sign = -1;
        break;
      }
    }
  }

  if (lscale == 0 && lfrac > ldot) {
    lfrac--;
    php_assert(lfrac == ldot);
  }

  if (sign < 0) {
    lhs[--lint] = '-';
  }

  if (lscale == scale || !add_trailing_zeroes) {
    return {lhs + lint, static_cast<string::size_type>(lfrac + lscale - lint)};
  } else {
    string result(lhs + lint, lfrac + lscale - lint);
    if (lscale == 0) {
      result.append(1, '.');
    }
    result.append(scale - lscale, '0');
    return result;
  }
}

string bc_add_positive(const char *lhs, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rint, int rdot, int rfrac, int rscale, int scale,
                       int sign) {
  int llen = ldot - lint;
  int rlen = rdot - rint;

  int resint, resdot, resfrac, resscale;

  int result_len = max(llen, rlen) + 1;
  int result_scale = max(lscale, rscale);
  int result_size = result_len + result_scale + 3;
  string result(result_size, false);

  int i, um = 0;
  int cur_pos = result_size;
  int was_frac = 0;
  for (i = result_scale - 1; i >= 0; i--) {
    if (i < lscale) {
      um += lhs[lfrac + i] - '0';
    }
    if (i < rscale) {
      um += rhs[rfrac + i] - '0';
    }

    if (um != 0 || was_frac) {
      result[--cur_pos] = (char)(um % 10 + '0');
      um /= 10;
      was_frac = 1;
    }
  }
  resscale = result_size - cur_pos;
  resfrac = cur_pos;
  if (was_frac) {
    result[--cur_pos] = '.';
  }
  resdot = cur_pos;

  for (int i = 0; i < result_len; i++) {
    if (i < llen) {
      um += lhs[ldot - i - 1] - '0';
    }
    if (i < rlen) {
      um += rhs[rdot - i - 1] - '0';
    }

    result[--cur_pos] = (char)(um % 10 + '0');
    um /= 10;
  }
  resint = cur_pos;
  php_assert(cur_pos > 0);

  return bc_round(result.buffer(), resint, resdot, resfrac, resscale, scale, sign, 1);
}

string bc_sub_positive(const char *lhs, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rint, int rdot, int rfrac, int rscale, int scale,
                       int sign) {
  int llen = ldot - lint;
  int rlen = rdot - rint;

  int resint, resdot, resfrac, resscale;

  int result_len = llen;
  int result_scale = max(lscale, rscale);
  int result_size = result_len + result_scale + 3;
  string result(result_size, false);

  int i, um = 0, next_um = 0;
  int cur_pos = result_size;
  int was_frac = 0;
  for (i = result_scale - 1; i >= 0; i--) {
    um = next_um;
    if (i < lscale) {
      um += lhs[lfrac + i] - '0';
    }
    if (i < rscale) {
      um -= rhs[rfrac + i] - '0';
    }
    if (um < 0) {
      next_um = -1;
      um += 10;
    } else {
      next_um = 0;
    }

    if (um != 0 || was_frac) {
      result[--cur_pos] = (char)(um + '0');
      was_frac = 1;
    }
  }
  resscale = result_size - cur_pos;
  resfrac = cur_pos;
  if (was_frac) {
    result[--cur_pos] = '.';
  }
  resdot = cur_pos;

  for (int i = 0; i < result_len; i++) {
    um = next_um;
    um += lhs[ldot - i - 1] - '0';
    if (i < rlen) {
      um -= rhs[rdot - i - 1] - '0';
    }
    if (um < 0) {
      next_um = -1;
      um += 10;
    } else {
      next_um = 0;
    }

    result[--cur_pos] = (char)(um + '0');
  }
  resint = cur_pos;
  php_assert(cur_pos > 0);

  return bc_round(result.buffer(), resint, resdot, resfrac, resscale, scale, sign, 1);
}

string bc_mul_positive_impl(const char *lhs, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rint, int rdot, int rfrac, int rscale, int scale,
                            int sign) {
  int llen = ldot - lint;
  int rlen = rdot - rint;

  int resint, resdot, resfrac, resscale;

  int result_len = llen + rlen;
  int result_scale = lscale + rscale;
  int result_size = result_len + result_scale + 3;
  string result(static_cast<string::size_type>(result_size), false);

  int *res = (int *)dl::allocate0(static_cast<size_t>(sizeof(int) * result_size));
  for (int i = -lscale; i < llen; i++) {
    int x = (i < 0 ? lhs[lfrac - i - 1] : lhs[ldot - i - 1]) - '0';
    for (int j = -rscale; j < rlen; j++) {
      int y = (j < 0 ? rhs[rfrac - j - 1] : rhs[rdot - j - 1]) - '0';
      res[i + j + result_scale] += x * y;
    }
  }
  for (int i = 0; i + 1 < result_size; i++) {
    res[i + 1] += res[i] / 10;
    res[i] %= 10;
  }

  int cur_pos = result_size;
  for (int i = 0; i < result_scale; i++) {
    result[--cur_pos] = (char)(res[i] + '0');
  }
  resscale = result_size - cur_pos;
  resfrac = cur_pos;
  if (result_scale > 0) {
    result[--cur_pos] = '.';
  }
  resdot = cur_pos;

  for (int i = result_scale; i < result_len + result_scale; i++) {
    result[--cur_pos] = (char)(res[i] + '0');
  }
  resint = cur_pos;
  php_assert(cur_pos > 0);

  dl::deallocate(res, static_cast<size_t>(sizeof(int) * result_size));

  return bc_round(result.buffer(), resint, resdot, resfrac, resscale, scale, sign, 1);
}

string bc_div_positive_impl(const char *lhs, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rint, int rdot, int rfrac, int rscale, int scale,
                            int sign) {
  int llen = ldot - lint;
  int rlen = rdot - rint;

  int resint, resdot = -1, resfrac = -1, resscale;

  int result_len = max(llen + rscale - rlen + 1, 1);
  int result_scale = scale;
  int result_size = result_len + result_scale + 3;

  if (rscale == 0 && rhs[rint] == '0') {
    php_warning("Division by zero in function bcdiv");
    return ZERO;
  }

  int dividend_len = llen + lscale;
  int divider_len = rlen + rscale;
  int *dividend = (int *)dl::allocate0(static_cast<size_t>(sizeof(int) * (result_size + dividend_len + divider_len)));
  int *divider = (int *)dl::allocate(static_cast<size_t>(sizeof(int) * divider_len));

  for (int i = -lscale; i < llen; i++) {
    int x = (i < 0 ? lhs[lfrac - i - 1] : lhs[ldot - i - 1]) - '0';
    dividend[llen - i - 1] = x;
  }

  for (int i = -rscale; i < rlen; i++) {
    int x = (i < 0 ? rhs[rfrac - i - 1] : rhs[rdot - i - 1]) - '0';
    divider[rlen - i - 1] = x;
  }

  int divider_skip = 0;
  while (divider_len > 0 && divider[0] == 0) {
    divider++;
    divider_skip++;
    divider_len--;
  }
  php_assert(divider_len > 0);

  int cur_pow = llen - rlen + divider_skip;
  int cur_pos = 2;

  if (cur_pow < -scale) {
    divider -= divider_skip;
    divider_len += divider_skip;
    dl::deallocate(dividend, static_cast<size_t>(sizeof(int) * (result_size + dividend_len + divider_len)));
    dl::deallocate(divider, static_cast<size_t>(sizeof(int) * divider_len));
    return bc_zero(scale);
  }

  string result(static_cast<string::size_type>(result_size), false);
  resint = cur_pos;
  if (cur_pow < 0) {
    result[cur_pos++] = '0';
    resdot = cur_pos;
    result[cur_pos++] = '.';
    resfrac = cur_pos;
    for (int i = -1; i > cur_pow; i--) {
      result[cur_pos++] = '0';
    }
  }

  int beg = 0, real_beg = 0;
  while (cur_pow >= -scale) {
    char dig = '0';
    while (true) {
      if (real_beg < beg && dividend[real_beg] == 0) {
        real_beg++;
      }

      bool less = false;
      if (real_beg == beg) {
        for (int i = 0; i < divider_len; i++) {
          if (dividend[beg + i] != divider[i]) {
            less = (dividend[beg + i] < divider[i]);
            break;
          }
        }
      }
      if (less) {
        break;
      }

      for (int i = divider_len - 1; i >= 0; i--) {
        dividend[beg + i] -= divider[i];
        if (dividend[beg + i] < 0) {
          dividend[beg + i] += 10;
          dividend[beg + i - 1]--;
        }
      }
      dig++;
    }

    result[cur_pos++] = dig;

    if (cur_pow == 0) {
      resdot = cur_pos;
      if (scale > 0) {
        result[cur_pos++] = '.';
      }
      resfrac = cur_pos;
    }
    cur_pow--;
    beg++;
  }
  resscale = cur_pos - resfrac;

  divider -= divider_skip;
  divider_len += divider_skip;
  dl::deallocate(dividend, static_cast<size_t>(sizeof(int) * (result_size + dividend_len + divider_len)));
  dl::deallocate(divider, static_cast<size_t>(sizeof(int) * divider_len));

  return bc_round(result.buffer(), resint, resdot, resfrac, resscale, scale, sign, 0);
}

string bc_add_impl(const char *lhs, int lsign, int lint, int ldot, int lfrac, int lscale, const char *rhs, int rsign, int rint, int rdot, int rfrac, int rscale,
                   int scale) {
  if (lsign > 0 && rsign > 0) {
    return bc_add_positive(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, scale, 1);
  }

  if (lsign > 0 && rsign < 0) {
    if (bc_comp_impl(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, 1000000000) >= 0) {
      return bc_sub_positive(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, scale, 1);
    } else {
      return bc_sub_positive(rhs, rint, rdot, rfrac, rscale, lhs, lint, ldot, lfrac, lscale, scale, -1);
    }
  }

  if (lsign < 0 && rsign > 0) {
    if (bc_comp_impl(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, 1000000000) <= 0) {
      return bc_sub_positive(rhs, rint, rdot, rfrac, rscale, lhs, lint, ldot, lfrac, lscale, scale, 1);
    } else {
      return bc_sub_positive(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, scale, -1);
    }
  }

  if (lsign < 0 && rsign < 0) {
    return bc_add_positive(lhs, lint, ldot, lfrac, lscale, rhs, rint, rdot, rfrac, rscale, scale, -1);
  }

  php_assert(0);
}

} // namespace legacy

struct BcNum {
  int n_sign{0};  // sign
  int n_int{0};   // number of trailing zeroes pluse one if there is '+' or '-' sign
  int n_dot{0};   // number of bytes prior dot
  int n_frac{0};  // n_dot + 1
  int n_scale{0}; // number of digits after dot
  string str;     // actual string representation
};

std::pair<BcNum, bool> bc_parse_number(const string &num) {
  BcNum bc_num;
  bc_num.str = num;
  bool success = legacy::bc_parse_number_impl(num, bc_num.n_sign, bc_num.n_int, bc_num.n_dot, bc_num.n_frac, bc_num.n_scale) >= 0;
  return {bc_num, success};
}

const BcNum ZERO_BC_NUM = bc_parse_number(ZERO).first;
const BcNum ONE_BC_NUM = bc_parse_number(ONE).first;

int bc_comp(const BcNum &lhs, const BcNum &rhs, int scale) {
  return legacy::bc_comp_impl(lhs.str.c_str(), lhs.n_int, lhs.n_dot, lhs.n_frac, lhs.n_scale, rhs.str.c_str(), rhs.n_int, rhs.n_dot, rhs.n_frac, rhs.n_scale,
                              scale);
}

string bc_mul_positive(const BcNum &lhs, const BcNum &rhs, int scale, int sign) {
  return legacy::bc_mul_positive_impl(lhs.str.c_str(), lhs.n_int, lhs.n_dot, lhs.n_frac, lhs.n_scale, rhs.str.c_str(), rhs.n_int, rhs.n_dot, rhs.n_frac,
                                      rhs.n_scale, scale, sign);
}

string bc_div_positive(const BcNum &lhs, const BcNum &rhs, int scale, int sign) {
  return legacy::bc_div_positive_impl(lhs.str.c_str(), lhs.n_int, lhs.n_dot, lhs.n_frac, lhs.n_scale, rhs.str.c_str(), rhs.n_int, rhs.n_dot, rhs.n_frac,
                                      rhs.n_scale, scale, sign);
}

string bc_add(const BcNum &lhs, const BcNum &rhs, int scale) {
  return legacy::bc_add_impl(lhs.str.c_str(), lhs.n_sign, lhs.n_int, lhs.n_dot, lhs.n_frac, lhs.n_scale, rhs.str.c_str(), rhs.n_sign, rhs.n_int, rhs.n_dot,
                             rhs.n_frac, rhs.n_scale, scale);
}

string bc_sub(const BcNum &lhs, const BcNum &rhs, int scale) {
  return legacy::bc_add_impl(lhs.str.c_str(), lhs.n_sign, lhs.n_int, lhs.n_dot, lhs.n_frac, lhs.n_scale, rhs.str.c_str(), (-1) * rhs.n_sign, rhs.n_int,
                             rhs.n_dot, rhs.n_frac, rhs.n_scale, scale);
}

} // namespace

void f$bcscale(int64_t scale) {
  if (scale < 0) {
    bc_scale = 0;
  } else {
    bc_scale = static_cast<int>(scale);
  }
}

string f$bcdiv(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcdiv", scale);
    scale = 0;
  }
  if (lhs_str.empty()) {
    return bc_zero(static_cast<int>(scale));
  }
  if (rhs_str.empty()) {
    php_warning("Division by empty string in function bcdiv");
    return bc_zero(static_cast<int>(scale));
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcdiv is not a number", lhs.str.c_str());
    return ZERO;
  }

  const auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcdiv is not a number", rhs.str.c_str());
    return ZERO;
  }

  return bc_div_positive(lhs, rhs, static_cast<int>(scale), lhs.n_sign * rhs.n_sign);
}

static string scale_num(const string &num, int64_t scale) {
  if (scale > 0) {
    string result = num;
    result.append(1, '.');
    result.append(scale, '0');
    return result;
  }
  return num;
}

string f$bcmod(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcmod", scale);
    scale = 0;
  }

  if (lhs_str.empty()) {
    return bc_zero(scale);
  }

  auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcmod is not a number", lhs.str.c_str());
    return ZERO;
  }

  auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcmod is not a number", rhs.str.c_str());
    return ZERO;
  }

  if (bc_comp(rhs, ZERO_BC_NUM, rhs.n_scale) == 0) {
    php_warning("bcmod(): Division by zero");
    return {};
  }

  const int rscale = std::max(lhs.n_scale, rhs.n_scale + static_cast<int>(scale));

  // result should have the same sign as lhs
  const int result_sign = lhs.n_sign;
  lhs.n_sign = 1;
  rhs.n_sign = 1;

  string div = bc_div_positive(lhs, rhs, 0, 1);
  BcNum x = bc_parse_number(div).first;

  string mul = bc_mul_positive(x, rhs, rscale, 1);
  x = bc_parse_number(mul).first;

  string sub = bc_sub(lhs, x, rscale);
  x = bc_parse_number(sub).first;

  return bc_div_positive(x, ONE_BC_NUM, scale, result_sign);
}

static std::pair<std::int64_t, bool> bc_num2int(const BcNum &num) {
  if (num.n_dot - num.n_int > 18) {
    return {0, false};
  }

  std::int64_t ingeger = 0;
  for (int i = num.n_int; i < num.n_dot; ++i) {
    ingeger = ingeger * 10 + num.str[i] - '0';
  }
  if (num.n_sign == -1) {
    ingeger *= -1;
  }

  return {ingeger, true};
}

string f$bcpow(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcpow", scale);
    scale = 0;
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str.empty() ? ZERO : lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcpow is not a number", lhs.str.c_str());
    return scale_num(ZERO, scale);
  }

  const auto [rhs, rhs_success] = bc_parse_number(rhs_str.empty() ? ZERO : rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcpow is not a number", rhs.str.c_str());
    return scale_num(ONE, scale);
  }
  if (rhs.n_scale != 0) {
    php_warning("bcpow(): non-zero scale \"%s\" in exponent", rhs.str.c_str());
  }

  auto [exponent, exp_success] = bc_num2int(rhs);
  if (!exp_success) {
    php_warning("Second parameter \"%s\" in function bcpow is larger than 1e18", rhs.str.c_str());
    return scale_num(ZERO, scale);
  }

  if (exponent == 0) {
    return scale_num(ONE, scale);
  }

  bool neg = false;
  int rscale = scale;

  if (exponent < 0) {
    neg = true;
    exponent = -exponent;
  } else {
    rscale = std::min(static_cast<int>(lhs.n_scale * exponent), std::max(static_cast<int>(scale), lhs.n_scale));
  }

  // set initial value of temp
  string power = lhs.str;
  int pwrscale = lhs.n_scale;
  while ((exponent & 1) == 0) {
    pwrscale = 2 * pwrscale;
    power = f$bcmul(power, power, pwrscale);
    exponent = exponent >> 1;
  }
  string temp = power;
  int calcscale = pwrscale;
  exponent = exponent >> 1;

  // do the calculation
  while (exponent > 0) {
    pwrscale = 2 * pwrscale;
    power = f$bcmul(power, power, pwrscale);
    if ((exponent & 1) == 1) {
      calcscale = pwrscale + calcscale;
      temp = f$bcmul(temp, power, calcscale);
    }
    exponent = exponent >> 1;
  }

  string result = temp;

  if (neg) {
    const BcNum &temp_bc_num = bc_parse_number(temp).first;
    if (bc_comp(temp_bc_num, ZERO_BC_NUM, temp_bc_num.n_scale) != 0) {
      result = f$bcdiv(ONE, temp, rscale);
    }
  }
  // just scale result
  return f$bcadd(result, ZERO, scale);
}

string f$bcadd(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (lhs_str.empty()) {
    return f$bcadd(ZERO, rhs_str, scale);
  }
  if (rhs_str.empty()) {
    return f$bcadd(lhs_str, ZERO, scale);
  }

  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcadd", scale);
    scale = 0;
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcadd is not a number", lhs.str.c_str());
    return bc_zero(static_cast<int>(scale));
  }

  const auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcadd is not a number", rhs.str.c_str());
    return bc_zero(static_cast<int>(scale));
  }

  return bc_add(lhs, rhs, static_cast<int>(scale));
}

string f$bcsub(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (lhs_str.empty()) {
    return f$bcsub(ZERO, rhs_str, scale);
  }
  if (rhs_str.empty()) {
    return f$bcsub(lhs_str, ZERO, scale);
  }

  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcsub", scale);
    scale = 0;
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcsub is not a number", lhs.str.c_str());
    return bc_zero(static_cast<int>(scale));
  }

  auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcsub is not a number", rhs.str.c_str());
    return bc_zero(static_cast<int>(scale));
  }

  rhs.n_sign *= -1;

  return bc_add(lhs, rhs, static_cast<int>(scale));
}

string f$bcmul(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (lhs_str.empty()) {
    return f$bcmul(ZERO, rhs_str, scale);
  }
  if (rhs_str.empty()) {
    return f$bcmul(lhs_str, ZERO, scale);
  }

  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcmul", scale);
    scale = 0;
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bcmul is not a number", lhs.str.c_str());
    return ZERO;
  }

  const auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bcmul is not a number", rhs.str.c_str());
    return ZERO;
  }

  return bc_mul_positive(lhs, rhs, static_cast<int>(scale), lhs.n_sign * rhs.n_sign);
}

int64_t f$bccomp(const string &lhs_str, const string &rhs_str, int64_t scale) {
  if (lhs_str.empty()) {
    return f$bccomp(ZERO, rhs_str, scale);
  }
  if (rhs_str.empty()) {
    return f$bccomp(lhs_str, ZERO, scale);
  }

  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bccomp", scale);
    scale = 0;
  }

  const auto [lhs, lhs_success] = bc_parse_number(lhs_str);
  if (!lhs_success) {
    php_warning("First parameter \"%s\" in function bccomp is not a number", lhs.str.c_str());
    return 0;
  }

  const auto [rhs, rhs_success] = bc_parse_number(rhs_str);
  if (!rhs_success) {
    php_warning("Second parameter \"%s\" in function bccomp is not a number", rhs.str.c_str());
    return 0;
  }

  if (lhs.n_sign != rhs.n_sign) {
    return (lhs.n_sign - rhs.n_sign) / 2;
  }

  return (1 - 2 * (lhs.n_sign < 0)) * bc_comp(lhs, rhs, static_cast<int>(scale));
}

// In some places we need to check if the number NUM is almost zero.
// Specifically, all but the last digit is 0 and the last digit is 1.
// Last digit is defined by scale.
static bool bc_is_near_zero(const BcNum &num, int scale) {
  if (scale > num.n_scale) {
    scale = num.n_scale;
  }

  const int len = num.n_dot - num.n_int;
  for (int i = num.n_int; i < len; ++i) {
    if (num.str[i] != '0') {
      return false;
    }
  }

  if (scale == 0) {
    return true;
  }

  const int count = scale - 1;
  const char *ptr = num.str.c_str() + num.n_frac;

  for (int i = 0; i < count; ++i) {
    if (ptr[i] != '0') {
      return false;
    }
  }

  return ptr[count] == '1' || ptr[count] == '0';
}

static const string ZERO_FIVE{"0.5"};
static const string TEN{"10"};

static const BcNum ZERO_FIVE_BC_NUM = bc_parse_number(ZERO_FIVE).first;

static std::pair<BcNum, int> bc_sqrt_calc_initial_guess(const BcNum &num, int cmp_with_one) {
  if (cmp_with_one < 0) {
    // the number is between 0 and 1. Guess should start at 1.
    return {ONE_BC_NUM, num.n_scale};
  }
  // the number is greater than 1. Guess should start at 10^(exp/2).
  BcNum exponent = bc_parse_number(string{num.n_dot}).first;
  string exponent_str = bc_mul_positive(exponent, ZERO_FIVE_BC_NUM, 0, 1);
  string guess_str = f$bcpow(TEN, exponent_str, 0);
  BcNum guess = bc_parse_number(guess_str).first;
  return {std::move(guess), 3};
}

static std::pair<string, bool> bc_sqrt(const BcNum &num, int scale) {
  // initial checks
  const int cmp_zero = bc_comp(num, ZERO_BC_NUM, num.n_scale);
  if (cmp_zero < 0 || num.n_sign == -1) {
    return {{}, false}; // error
  } else if (cmp_zero == 0) {
    return {ZERO, true};
  }

  const int cmp_one = bc_comp(num, ONE_BC_NUM, num.n_scale);
  if (cmp_one == 0) {
    return {ONE, true};
  }

  // calculate the initial guess.
  auto [guess, cscale] = bc_sqrt_calc_initial_guess(num, cmp_one);
  const int rscale = scale > num.n_scale ? scale : num.n_scale;

  // find the square root using Newton's algorithm.
  for (;;) {
    const BcNum prev_guess = guess;

    string div_str = bc_div_positive(num, guess, cscale, 1);
    guess = bc_parse_number(div_str).first;

    string add_str = bc_add(guess, prev_guess, cscale);
    guess = bc_parse_number(add_str).first;

    string mul_str = bc_mul_positive(guess, ZERO_FIVE_BC_NUM, cscale, 1);
    guess = bc_parse_number(mul_str).first;

    string sub_str = bc_sub(guess, prev_guess, cscale + 1);
    BcNum diff = bc_parse_number(sub_str).first;

    if (bc_is_near_zero(diff, cscale)) {
      if (cscale < rscale + 1) {
        cscale = std::min(cscale * 3, rscale + 1);
      } else {
        break;
      }
    }
  }

  return {bc_div_positive(guess, ONE_BC_NUM, rscale, 1), true};
}

string f$bcsqrt(const string &num_str, int64_t scale) {
  if (scale == std::numeric_limits<int64_t>::min()) {
    scale = bc_scale;
  }
  if (scale < 0) {
    php_warning("Wrong parameter scale = %" PRIi64 " in function bcsqrt", scale);
    scale = 0;
  }
  if (num_str.empty()) {
    return bc_zero(static_cast<int>(scale));
  }

  const auto [num, parse_success] = bc_parse_number(num_str);
  if (!parse_success) {
    php_warning("First parameter \"%s\" in function bcsqrt is not a number", num.str.c_str());
    return bc_zero(static_cast<int>(scale));
  }

  auto [sqrt, sqrt_success] = bc_sqrt(num, scale);
  if (!sqrt_success) {
    php_warning("bcsqrt(): Square root of negative number");
    return {};
  }

  const BcNum &sqrt_bc_num = bc_parse_number(sqrt).first;
  return bc_div_positive(sqrt_bc_num, ONE_BC_NUM, scale, 1);
}

void free_bcmath_lib() {
  bc_scale = 0;
}
