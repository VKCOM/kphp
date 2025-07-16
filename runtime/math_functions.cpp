// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/math_functions.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <random>
#include <sys/time.h>

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <sys/random.h>
#endif

#include "common/cycleclock.h"
#include "runtime-common/stdlib/math/random-functions.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "server/php-engine-vars.h"

namespace {
int64_t secure_rand_buf(char* const buf, int64_t length) noexcept {
#if defined(__APPLE__)
  arc4random_buf(static_cast<void*>(buf), static_cast<size_t>(length));
  return 0;
#else
  return getrandom(buf, static_cast<size_t>(length), 0x0);
#endif
}
} // namespace

int64_t f$bindec(const string& number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    const char c = number[i];
    if (likely(vk::any_of_equal(c, '0', '1'))) {
      v = math_functions_impl_::mult_and_add<2>(v, static_cast<uint8_t>(c - '0'), overflow);
    } else {
      bad_str_param = true;
    }
  }

  if (unlikely(bad_str_param)) {
    php_warning("Wrong parameter '%s' in function bindec", number.c_str());
  }
  if (unlikely(overflow)) {
    php_warning("Integer overflow on converting '%s' in function bindec, "
                "the result will be different from PHP",
                number.c_str());
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

double f$lcg_value() {
  dl::enter_critical_section(); // OK

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
#define MODMULT(a, b, c, m, s)                                                                                                                                 \
  q = s / a;                                                                                                                                                   \
  s = b * (s - a * q) - c * q;                                                                                                                                 \
  if (s < 0) {                                                                                                                                                 \
    s += m;                                                                                                                                                    \
  }
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
  static MTRandGenerator& get() {
    static MTRandGenerator generator;
    return generator;
  }

  void lazy_init() noexcept {
    if (unlikely(!gen_)) {
      const uint64_t s = (static_cast<uint64_t>(pid) << 32) ^ cycleclock_now();
      gen_ = new (&gen_storage_) std::mt19937_64{s};
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
  std::mt19937_64* gen_{nullptr};
};

} // namespace

void f$mt_srand(int64_t seed) noexcept {
  if (seed == std::numeric_limits<int64_t>::min()) {
    seed = std::chrono::steady_clock::now().time_since_epoch().count();
  }
  MTRandGenerator::get().set_seed(seed);
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

Optional<int64_t> f$random_int(int64_t l, int64_t r) noexcept {
  if (unlikely(l > r)) {
    php_warning("Argument #1 ($min) must be less than or equal to argument #2 ($max)");
    return false;
  }

  if (unlikely(l == r)) {
    return l;
  }

  try {
    std::random_device rd{"/dev/urandom"};
    std::uniform_int_distribution dist{l, r};

    return dist(rd);
  } catch (const std::exception& e) {
    php_warning("Source of randomness cannot be found: %s", e.what());
    return false;
  } catch (...) {
    php_critical_error("Unhandled exception");
  }
}

Optional<string> f$random_bytes(int64_t length) noexcept {
  if (unlikely(length < 1)) {
    php_warning("Argument #1 ($length) must be greater than 0");
    return false;
  }

  string str{static_cast<string::size_type>(length), false};

  if (secure_rand_buf(str.buffer(), static_cast<size_t>(length)) == -1) {
    php_warning("Source of randomness cannot be found: %s", std::strerror(errno));
    return false;
  }

  return str;
}

void init_math_functions() noexcept {
  MTRandGenerator::get().lazy_init();
}
