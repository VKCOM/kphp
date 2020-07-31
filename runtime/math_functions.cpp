#include "runtime/math_functions.h"

#include <sys/time.h> //gettimeofday

#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

int64_t f$bindec(const string &number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    const char c = number[i];
    if (likely(vk::any_of_equal(c, '0', '1'))) {
      v = v * 2 + c - '0';
    } else {
      bad_str_param = true;
    }
    if (unlikely(v > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
      overflow = true;
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

  return string(s + i, static_cast<string::size_type>(65 - i));
}

string f$dechex(int64_t number) noexcept {
  auto v = static_cast<uint64_t>(number);

  char s[17];
  int i = 16;

  do {
    s[--i] = lhex_digits[v & 15];
    v >>= 4;
  } while (v > 0);

  return string(s + i, 16 - i);
}

int64_t f$hexdec(const string &number) noexcept {
  uint64_t v = 0;
  bool bad_str_param = number.empty();
  bool overflow = false;
  for (string::size_type i = 0; i < number.size(); i++) {
    char c = number[i];
    if ('0' <= c && c <= '9') {
      v = v * 16 + c - '0';
    } else {
      c |= 0x20;
      if (likely('a' <= c && c <= 'f')) {
        v = v * 16 + c - 'a' + 10;
      } else {
        bad_str_param = true;
      }
    }
    if (unlikely(v > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
      overflow = true;
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
    s2 = (int)getpid();

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


static int make_seed() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_REALTIME, &T) >= 0);
  return ((int)T.tv_nsec * 123456789) ^ ((int)T.tv_sec * 987654321);
}

void f$srand(int64_t seed) {
  if (seed == std::numeric_limits<int64_t>::min()) {
    seed = make_seed();
  }
  srand(static_cast<uint32_t>(seed));
}

int64_t f$rand() {
  return rand();
}

// TODO FIX
// REMOVE INT
int64_t f$rand(int64_t l_i64, int64_t r_i64) {
  auto l = static_cast<int>(l_i64);
  auto r = static_cast<int>(r_i64);
  if (l > r) {
    return 0;
  }
  unsigned int diff = (unsigned int)r - (unsigned int)l + 1u;
  int64_t shift = 0;
  if (RAND_MAX == 0x7fffffff && diff == 0) { // l == MIN_INT, r == MAX_INT, RAND_MAX == MAX_INT
    shift = f$rand(0, RAND_MAX) * 2u + (rand() & 1);
  } else if (diff <= RAND_MAX + 1u) {
    unsigned int upper_bound = ((RAND_MAX + 1u) / diff) * diff;
    unsigned int r;
    do {
      r = rand();
    } while (r > upper_bound);
    shift = r % diff;
  } else {
    shift = f$rand(0, (diff >> 1) - 1) * 2u + (rand() & 1);
  }
  return l + shift;
}

int64_t f$getrandmax() {
  return RAND_MAX;
}

void f$mt_srand(int64_t seed) {
  f$srand(seed);
}

int64_t f$mt_rand() {
  return rand();
}

int64_t f$mt_rand(int64_t l, int64_t r) {
  return f$rand(l, r);
}

int64_t f$mt_getrandmax() {
  return RAND_MAX;
}


var f$abs(const var &v) {
  var num = v.to_numeric();
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
    php_warning("Wrong parameter frombase (%ld) in function base_convert", frombase);
    return number;
  }
  if (tobase < 2 || tobase > 36) {
    php_warning("Wrong parameter tobase (%ld) in function base_convert", tobase);
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
    php_warning("Wrong parameter precision (%ld) in function round", precision);
    return v;
  }

  double mul = pow(10.0, (double)precision);
  return round(v * mul) / mul;
}
