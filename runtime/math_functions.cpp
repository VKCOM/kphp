// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/math_functions.h"

#include <chrono>
#include <random>
#include <sys/time.h>

#include "common/cycleclock.h"
#include "runtime/critical_section.h"
#include "runtime/string_functions.h"
#include "server/php-engine-vars.h"

namespace {
  template<uint8_t M>
  uint64_t mult_and_add(uint64_t x, uint8_t y, bool &overflow) noexcept {
    const uint64_t r = x * M + y;
    overflow = overflow || r < x || r > static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
    return r;
  }
} // namespace

int64_t f$bindec(const string &number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    const char c = number[i];
    if (likely(vk::any_of_equal(c, '0', '1'))) {
      v = mult_and_add<2>(v, static_cast<uint8_t>(c - '0'), overflow);
    } else {
      bad_str_param = true;
    }
  }

  if (unlikely(bad_str_param)) {
    php_warning("Wrong parameter '%s' in function bindec", number.c_str());
  }
  if (unlikely(overflow)) {
    php_warning("Integer overflow on converting '%s' in function bindec, "
                "the result will be different from PHP", number.c_str());
  }
  return static_cast<int64_t>(v);
}

string f$decbin(int64_t number) noexcept {
  auto v = static_cast<uint64_t>(number);

  char s[66];
  int i = 65;

  do {
    s[--i] = static_cast<char>((v & 1) + '0');
    v >>= 1;
  } while (v > 0);

  return {s + i, static_cast<string::size_type>(65 - i)};
}

string f$dechex(int64_t number) noexcept {
  auto v = static_cast<uint64_t>(number);

  char s[17];
  int i = 16;

  do {
    s[--i] = lhex_digits[v & 15];
    v >>= 4;
  } while (v > 0);

  return {s + i, static_cast<string::size_type>(16 - i)};
}

int64_t f$hexdec(const string &number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    const uint8_t d = hex_to_int(number[i]);
    if (unlikely(d == 16)) {
      bad_str_param = true;
    } else {
      v = mult_and_add<16>(v, d, overflow);
    }
  }

  if (unlikely(bad_str_param)) {
    php_warning("Wrong parameter \"%s\" in function hexdec", number.c_str());
  }
  if (unlikely(overflow)) {
    php_warning("Integer overflow on converting '%s' in function hexdec, "
                "the result will be different from PHP", number.c_str());
  }
  return v;
}

double f$lcg_value() {
  dl::enter_critical_section();//OK

  static long long lcg_value_last_query_num = -1;
  static int s1 = 0, s2 = 0;

  if (dl::query_num != lcg_value_last_query_num) {
    struct timeval tv;

    if (gettimeofday(&tv, nullptr) == 0) {
      s1 = (int)tv.tv_sec ^ ((int)tv.tv_usec << 11);
    } else {
      s1 = 1;
    }
    s2 = pid;

    if (gettimeofday(&tv, nullptr) == 0) {
      s2 ^= (int)(tv.tv_usec << 11);
    }

    lcg_value_last_query_num = dl::query_num;
  }

  int q;
#define MODMULT(a, b, c, m, s) q = s / a; s = b * (s - a * q) - c * q; if (s < 0) {s += m;}
  MODMULT(53668, 40014, 12211, 2147483563, s1);
  MODMULT(52774, 40692, 3791, 2147483399, s2);
#undef MODMULT

  int z = s1 - s2;
  if (z < 1) {
    z += 2147483562;
  }

  dl::leave_critical_section();
  return z * 4.656613e-10;
}

namespace {

class MTRandGenerator : vk::not_copyable {
public:
  static MTRandGenerator &get() {
    static MTRandGenerator generator;
    return generator;
  }

  void lazy_init() noexcept {
    if (unlikely(!gen_)) {
      const uint64_t s = (static_cast<uint64_t>(pid) << 32) ^ cycleclock_now();
      gen_ = new(&gen_storage_) std::mt19937_64{s};
    }
  }

  void set_seed(int64_t s) noexcept {
    php_assert(gen_);
    gen_->seed(static_cast<std::mt19937_64::result_type>(s));
  }

  int64_t generate_random(int64_t l, int64_t r) noexcept {
    if (unlikely(l > r)) {
      return 0;
    }
    php_assert(gen_);
    return std::uniform_int_distribution<int64_t>{l, r}(*gen_);
  }

private:
  MTRandGenerator() = default;

  std::aligned_storage_t<sizeof(std::mt19937_64), alignof(std::mt19937_64)> gen_storage_{};
  std::mt19937_64 *gen_{nullptr};
};

} // namespace

void f$mt_srand(int64_t seed) noexcept {
  if (seed == std::numeric_limits<int64_t>::min()) {
    seed = std::chrono::steady_clock::now().time_since_epoch().count();
  }
  MTRandGenerator::get().set_seed(seed);
}

int64_t f$mt_getrandmax() noexcept {
  // PHP uses this value, but it doesn't forbid the users to pass
  // a number that exceeds this limit into mt_rand()
  return std::numeric_limits<int32_t>::max();
}

int64_t f$mt_rand() noexcept {
  return f$mt_rand(0, f$mt_getrandmax());
}

int64_t f$mt_rand(int64_t l, int64_t r) noexcept {
  return MTRandGenerator::get().generate_random(l, r);
}

void f$srand(int64_t seed) noexcept {
  f$mt_srand(seed);
}

int64_t f$rand() noexcept {
  return f$mt_rand();
}

int64_t f$rand(int64_t l, int64_t r) noexcept {
  return f$mt_rand(l, r);
}

int64_t f$getrandmax() noexcept {
  return f$mt_getrandmax();
}

mixed f$abs(const mixed &v) {
  mixed num = v.to_numeric();
  if (num.is_int()) {
    return std::abs(num.to_int());
  }
  return fabs(num.to_float());
}

int64_t f$abs(int64_t v) {
  return std::abs(v);
}

double f$abs(double v) {
  return std::abs(v);
}

int64_t f$abs(const Optional<int64_t> &v) {
  return f$abs(val(v));
}

int64_t f$abs(const Optional<bool> &v) {
  return f$abs(static_cast<int64_t>(val(v)));
}

double f$abs(const Optional<double> &v) {
  return f$abs(val(v));
}



string f$base_convert(const string &number, int64_t frombase, int64_t tobase) {
  if (frombase < 2 || frombase > 36) {
    php_warning("Wrong parameter frombase (%" PRIi64 ") in function base_convert", frombase);
    return number;
  }
  if (tobase < 2 || tobase > 36) {
    php_warning("Wrong parameter tobase (%" PRIi64 ") in function base_convert", tobase);
    return number;
  }

  string::size_type len = number.size();
  string::size_type f = 0;
  string result;
  if (number[0] == '-' || number[0] == '+') {
    f++;
    len--;
    if (number[0] == '-') {
      result.push_back('-');
    }
  }
  if (len == 0) {
    php_warning("Wrong parameter number (%s) in function base_convert", number.c_str());
    return number;
  }

  const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

  string n(len, false);
  for (string::size_type i = 0; i < len; i++) {
    const char *s = (const char *)memchr(digits, tolower(number[i + f]), 36);
    if (s == nullptr || s - digits >= frombase) {
      php_warning("Wrong character '%c' at position %u in parameter number (%s) in function base_convert", number[i + f], i + f, number.c_str());
      return number;
    }
    n[i] = (char)(s - digits);
  }

  int64_t um = 0;
  string::size_type st = 0;
  while (st < len) {
    um = 0;
    for (string::size_type i = st; i < len; i++) {
      um = um * frombase + n[i];
      n[i] = (char)(um / tobase);
      um %= tobase;
    }
    while (st < len && n[st] == 0) {
      st++;
    }
    result.push_back(digits[um]);
  }

  string::size_type i = f;
  int64_t j = int64_t{result.size()} - 1;
  while (i < j) {
    swap(result[i++], result[static_cast<string::size_type>(j--)]);
  }

  return result;
}

double f$round(double v, int64_t precision) {
  if (std::abs(precision) > 100) {
    php_warning("Wrong parameter precision (%" PRIi64 ") in function round", precision);
    return v;
  }

  double mul = pow(10.0, (double)precision);
  return round(v * mul) / mul;
}

void init_math_functions() noexcept {
  MTRandGenerator::get().lazy_init();
}
