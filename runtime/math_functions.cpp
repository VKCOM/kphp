#include "runtime/math_functions.h"

#include <sys/time.h> //gettimeofday

#include "runtime/string_functions.h"

int f$bindec(const string &number) {
  unsigned int v = 0;
  bool need_warning = number.empty();
  for (int i = 0; i < (int)number.size(); i++) {
    char c = number[i];
    if (v >= 0x80000000) {
      need_warning = true;
    }
    if ('0' <= c && c <= '1') {
      v = v * 2 + c - '0';
    } else {
      need_warning = true;
    }
  }

  if (need_warning) {
    php_warning("Wrong parameter \"%s\" in function bindec", number.c_str());
  }
  return (int)v;
}

string f$decbin(int number) {
  unsigned int v = number;

  char s[66];
  int i = 65;

  do {
    s[--i] = (char)((v & 1) + '0');
    v >>= 1;
  } while (v > 0);

  return string(s + i, (dl::size_type)(65 - i));
}

string f$dechex(int number) {
  unsigned int v = number;

  char s[17];
  int i = 16;

  do {
    s[--i] = lhex_digits[v & 15];
    v >>= 4;
  } while (v > 0);

  return string(s + i, 16 - i);
}

int f$hexdec(const string &number) {
  unsigned int v = 0;
  bool need_warning = number.empty();
  for (int i = 0; i < (int)number.size(); i++) {
    char c = number[i];
    if (v >= 0x10000000) {
      need_warning = true;
    }
    if ('0' <= c && c <= '9') {
      v = v * 16 + c - '0';
    } else {
      c |= 0x20;
      if ('a' <= c && c <= 'f') {
        v = v * 16 + c - 'a' + 10;
      } else {
        need_warning = true;
      }
    }
  }

  if (need_warning) {
    php_warning("Wrong parameter \"%s\" in function hexdec", number.c_str());
  }
  return (int)v;
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

void f$srand(int seed) {
  if (seed == INT_MIN) {
    seed = make_seed();
  }
  srand(seed);
}

int f$rand() {
  return rand();
}

int f$rand(int l, int r) {
  if (l > r) {
    return 0;
  }
  unsigned int diff = (unsigned int)r - (unsigned int)l + 1u;
  unsigned int shift;
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

int f$getrandmax() {
  return RAND_MAX;
}

void f$mt_srand(int seed) {
  if (seed == INT_MIN) {
    seed = make_seed();
  }
  srand(seed);
}

int f$mt_rand() {
  return rand();
}

int f$mt_rand(int l, int r) {
  return f$rand(l, r);
}

int f$mt_getrandmax() {
  return RAND_MAX;
}


var f$min(const var &a) {
  return f$min(a.as_array("min", 1));
}

var f$max(const var &a) {
  return f$max(a.as_array("max", 1));
}


var f$abs(const var &v) {
  var num = v.to_numeric();
  if (num.is_int()) {
    return abs(num.to_int());
  }
  return fabs(num.to_float());
}

string f$base_convert(const string &number, int frombase, int tobase) {
  if (frombase < 2 || frombase > 36) {
    php_warning("Wrong parameter frombase (%d) in function base_convert", frombase);
    return number;
  }
  if (tobase < 2 || tobase > 36) {
    php_warning("Wrong parameter tobase (%d) in function base_convert", tobase);
    return number;
  }

  int l = number.size(), f = 0;
  string result;
  if (number[0] == '-' || number[0] == '+') {
    f++;
    l--;
    if (number[0] == '-') {
      result.push_back('-');
    }
  }
  if (l == 0) {
    php_warning("Wrong parameter number (%s) in function base_convert", number.c_str());
    return number;
  }

  const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

  string n(l, false);
  for (int i = 0; i < l; i++) {
    const char *s = (const char *)memchr(digits, tolower(number[i + f]), 36);
    if (s == nullptr || (int)(s - digits) >= frombase) {
      php_warning("Wrong character '%c' at position %d in parameter number (%s) in function base_convert", number[i + f], i + f, number.c_str());
      return number;
    }
    n[i] = (char)(s - digits);
  }

  int um, st = 0;
  while (st < l) {
    um = 0;
    for (int i = st; i < l; i++) {
      um = um * frombase + n[i];
      n[i] = (char)(um / tobase);
      um %= tobase;
    }
    while (st < l && n[st] == 0) {
      st++;
    }
    result.push_back(digits[um]);
  }

  int i = f, j = (int)result.size() - 1;
  while (i < j) {
    swap(result[i++], result[j--]);
  }

  return result;
}

int pow_int(int x, int y) {
  int res = 1;
  while (y > 0) {
    if (y & 1) {
      res *= x;
    }
    x *= x;
    y >>= 1;
  }
  return res;
}

double pow_float(double x, double y) {
  if (x < 0.0) {
    php_warning("Calculating pow with negative base and double exp will produce zero");
    return 0.0;
  }

  if (x == 0.0) {
    return y == 0.0;
  }

  return pow(x, y);
}

var f$pow(const var &num, const var &deg) {
  if (num.is_int() && deg.is_int() && deg.to_int() >= 0) {
    return pow_int(num.to_int(), deg.to_int());
  } else {
    return pow_float(num.to_float(), deg.to_float());
  }
}

double f$round(double v, int precision) {
  if (abs(precision) > 100) {
    php_warning("Wrong parameter precision (%d) in function round", precision);
    return v;
  }

  double mul = pow(10.0, (double)precision);
  return round(v * mul) / mul;
}
