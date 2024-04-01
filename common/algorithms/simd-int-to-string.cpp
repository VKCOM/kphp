// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/simd-int-to-string.h"

// Apple M1 doesn't have SIMD perfectly equal to x86,
// we can't include <emmintrin.h>, use _mm_srli_si128 and lots of others.
// We have two options:
// 1) copy emmintrin.h from https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h to the current repo
//    and use it instead of a system one
// 2) don't use simd at all: make this file just compile and work, but without simd actually
// The second option is chosen for now, as M1 is just a target for development.
#ifdef __x86_64__

// based on
// https://github.com/miloyip/itoa-benchmark/blob/master/src/sse2.cpp

#include <cassert>
#include <cstdint>
#include <emmintrin.h>

#include "common/algorithms/fastmod.h"

namespace impl_ {
inline char lookup_digit_table(size_t x) noexcept {
  static constexpr char digit_table[200] = {'0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9', '1', '0', '1',
                                            '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0', '2', '1', '2', '2',
                                            '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9', '3', '0', '3', '1', '3', '2', '3', '3', '3',
                                            '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5',
                                            '4', '6', '4', '7', '4', '8', '4', '9', '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5',
                                            '7', '5', '8', '5', '9', '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8',
                                            '6', '9', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '8',
                                            '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9', '9', '0', '9', '1',
                                            '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'};
  return digit_table[x];
}

inline __m128i convert_8digits_to_128bits_vector(uint32_t value) noexcept {
  // abcd, efgh = abcdefgh divmod 10000
  const __m128i abcdefgh = _mm_cvtsi32_si128(value);

  constexpr uint32_t div_10000 = 0xd1b71759;
  const __m128i div_10000_vector = _mm_set_epi32(div_10000, div_10000, div_10000, div_10000);
  const __m128i abcd = _mm_srli_epi64(_mm_mul_epu32(abcdefgh, div_10000_vector), 45);

  const __m128i _10000_vector = _mm_set_epi32(10000, 10000, 10000, 10000);
  const __m128i efgh = _mm_sub_epi32(abcdefgh, _mm_mul_epu32(abcd, _10000_vector));

  // v1 = [ abcd, efgh, 0, 0, 0, 0, 0, 0 ]
  const __m128i v1 = _mm_unpacklo_epi16(abcd, efgh);
  // v1a = v1 * 4 = [ abcd * 4, efgh * 4, 0, 0, 0, 0, 0, 0 ]
  const __m128i v1a = _mm_slli_epi64(v1, 2);

  // v2 = [ abcd * 4, abcd * 4, abcd * 4, abcd * 4, efgh * 4, efgh * 4, efgh * 4, efgh * 4 ]
  const __m128i v2a = _mm_unpacklo_epi16(v1a, v1a);
  const __m128i v2 = _mm_unpacklo_epi32(v2a, v2a);

  // 10^3, 10^2, 10^1, 10^0
  const __m128i div_powers_vector = _mm_set_epi16(-32768, 13108, 5243, 8389, -32768, 13108, 5243, 8389);
  // v4 = v2 div 10^3, 10^2, 10^1, 10^0 = [ a, ab, abc, abcd, e, ef, efg, efgh ]
  const __m128i v3 = _mm_mulhi_epu16(v2, div_powers_vector);

  const __m128i shift_powers_vector = _mm_set_epi16(-32768, 8192, 2048, 128, -32768, 8192, 2048, 128);
  const __m128i v4 = _mm_mulhi_epu16(v3, shift_powers_vector);

  const __m128i _10_vector = _mm_set_epi16(10, 10, 10, 10, 10, 10, 10, 10);
  // v5 = v4 * 10 = [ a0, ab0, abc0, abcd0, e0, ef0, efg0, efgh0 ]
  const __m128i v5 = _mm_mullo_epi16(v4, _10_vector);

  // v6 = v5 << 16 = [ 0, a0, ab0, abc0, 0, e0, ef0, efg0 ]
  const __m128i v6 = _mm_slli_epi64(v5, 16);

  // v7 = v4 - v6 = { a, b, c, d, e, f, g, h }
  const __m128i v7 = _mm_sub_epi16(v4, v6);

  return v7;
}

inline __m128i shift_digits(__m128i a, unsigned digit) noexcept {
  switch (digit) {
    case 0:
      return a;
    case 1:
      return _mm_srli_si128(a, 1);
    case 2:
      return _mm_srli_si128(a, 2);
    case 3:
      return _mm_srli_si128(a, 3);
    case 4:
      return _mm_srli_si128(a, 4);
    case 5:
      return _mm_srli_si128(a, 5);
    case 6:
      return _mm_srli_si128(a, 6);
    case 7:
      return _mm_srli_si128(a, 7);
    case 8:
      return _mm_srli_si128(a, 8);
  }
  assert(0);
}

inline char *uint32_less10000_to_string(uint32_t value, char *out_buffer) {
  if (value < 10U) {
    *out_buffer = static_cast<char>('0' + static_cast<char>(value));
    return out_buffer + 1;
  }

  const uint32_t d1 = fastmod::fastdiv<100U>(value) << 1U;
  const uint32_t d2 = fastmod::fastmod<100U>(value) << 1U;

  if (value >= 1000U) {
    *out_buffer++ = lookup_digit_table(d1);
  }
  if (value >= 100U) {
    *out_buffer++ = lookup_digit_table(d1 + 1);
  }
  *out_buffer++ = lookup_digit_table(d2);
  *out_buffer++ = lookup_digit_table(d2 + 1);
  return out_buffer;
}

inline char *uint32_greater10000_less100000000_to_string(uint32_t value, char *out_buffer) {
  // value = bbbbcccc
  const uint32_t b = fastmod::fastdiv<10000U>(value);
  const uint32_t c = fastmod::fastmod<10000U>(value);

  const uint32_t d1 = fastmod::fastdiv<100U>(b) << 1U;
  const uint32_t d2 = fastmod::fastmod<100U>(b) << 1U;

  const uint32_t d3 = fastmod::fastdiv<100U>(c) << 1U;
  const uint32_t d4 = fastmod::fastmod<100U>(c) << 1U;

  if (value >= 10000000U) {
    *out_buffer++ = lookup_digit_table(d1);
  }
  if (value >= 1000000U) {
    *out_buffer++ = lookup_digit_table(d1 + 1);
  }
  if (value >= 100000U) {
    *out_buffer++ = lookup_digit_table(d2);
  }
  *out_buffer++ = lookup_digit_table(d2 + 1);

  *out_buffer++ = lookup_digit_table(d3);
  *out_buffer++ = lookup_digit_table(d3 + 1);
  *out_buffer++ = lookup_digit_table(d4);
  *out_buffer++ = lookup_digit_table(d4 + 1);
  return out_buffer;
}
} // namespace impl_

char *simd_uint32_to_string(uint32_t value, char *out_buffer) noexcept {
  if (value < 10000U) {
    return impl_::uint32_less10000_to_string(value, out_buffer);
  }
  if (value < 100000000U) {
    return impl_::uint32_greater10000_less100000000_to_string(value, out_buffer);
  }

  // value = aabbbbbbbb in decimal
  const uint32_t a = fastmod::fastdiv<100000000U>(value); // 1 to 42
  value = fastmod::fastmod<100000000U>(value);

  if (a >= 10) {
    const unsigned i = a << 1;
    *out_buffer++ = impl_::lookup_digit_table(i);
    *out_buffer++ = impl_::lookup_digit_table(i + 1);
  } else {
    *out_buffer++ = static_cast<char>('0' + static_cast<char>(a));
  }

  const __m128i b = impl_::convert_8digits_to_128bits_vector(value);

  const __m128i ascii_zero = _mm_set_epi8('0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');
  const __m128i ba = _mm_add_epi8(_mm_packus_epi16(_mm_setzero_si128(), b), ascii_zero);
  const __m128i result = _mm_srli_si128(ba, 8);
  _mm_storel_epi64(reinterpret_cast<__m128i *>(out_buffer), result);
  return out_buffer + 8;
}

char *simd_int32_to_string(int32_t value, char *out_buffer) noexcept {
  auto u = static_cast<uint32_t>(value);
  if (value < 0) {
    *out_buffer++ = '-';
    u = ~u + 1;
  }
  return simd_uint32_to_string(u, out_buffer);
}

char *simd_uint64_to_string(uint64_t value, char *out_buffer) {
  if (value < 10000U) {
    return impl_::uint32_less10000_to_string(static_cast<uint32_t>(value), out_buffer);
  }
  if (value < 100000000U) {
    return impl_::uint32_greater10000_less100000000_to_string(static_cast<uint32_t>(value), out_buffer);
  }

  if (value < 10000000000000000ULL) {
    const auto v0 = static_cast<uint32_t>(value / 100000000U);
    const auto v1 = static_cast<uint32_t>(value % 100000000U);

    const __m128i a0 = impl_::convert_8digits_to_128bits_vector(v0);
    const __m128i a1 = impl_::convert_8digits_to_128bits_vector(v1);

    const __m128i ascii_zero = _mm_set_epi8('0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

    // Convert to bytes, add '0'
    const __m128i va = _mm_add_epi8(_mm_packus_epi16(a0, a1), ascii_zero);

    // Count number of digit
    const unsigned mask = _mm_movemask_epi8(_mm_cmpeq_epi8(va, ascii_zero));

    unsigned digit = __builtin_ctz(~mask | 0x8000);

    // Shift digits to the beginning
    __m128i result = impl_::shift_digits(va, digit);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out_buffer), result);
    return out_buffer + 16 - digit;
  }

  const auto a = static_cast<uint32_t>(value / 10000000000000000ULL); // 1 to 1844
  value %= 10000000000000000ULL;

  if (a < 10U) {
    *out_buffer++ = static_cast<char>('0' + static_cast<char>(a));
  } else if (a < 100U) {
    const uint32_t i = a << 1U;
    *out_buffer++ = impl_::lookup_digit_table(i);
    *out_buffer++ = impl_::lookup_digit_table(i + 1);
  } else if (a < 1000U) {
    *out_buffer++ = static_cast<char>('0' + static_cast<char>(fastmod::fastdiv<100U>(a)));

    const uint32_t i = fastmod::fastmod<100U>(a) << 1U;
    *out_buffer++ = impl_::lookup_digit_table(i);
    *out_buffer++ = impl_::lookup_digit_table(i + 1);
  } else {
    const uint32_t i = fastmod::fastdiv<100U>(a) << 1U;
    const uint32_t j = fastmod::fastmod<100U>(a) << 1U;
    *out_buffer++ = impl_::lookup_digit_table(i);
    *out_buffer++ = impl_::lookup_digit_table(i + 1);
    *out_buffer++ = impl_::lookup_digit_table(j);
    *out_buffer++ = impl_::lookup_digit_table(j + 1);
  }

  const auto v0 = static_cast<uint32_t>(value / 100000000U);
  const auto v1 = static_cast<uint32_t>(value % 100000000U);

  const __m128i a0 = impl_::convert_8digits_to_128bits_vector(v0);
  const __m128i a1 = impl_::convert_8digits_to_128bits_vector(v1);
  const __m128i ascii_zero = _mm_set_epi8('0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

  // Convert to bytes, add '0'
  const __m128i va = _mm_add_epi8(_mm_packus_epi16(a0, a1), ascii_zero);
  _mm_storeu_si128(reinterpret_cast<__m128i *>(out_buffer), va);
  return out_buffer + 16;
}

char *simd_int64_to_string(int64_t value, char *out_buffer) {
  auto u = static_cast<uint64_t>(value);
  if (value < 0) {
    *out_buffer++ = '-';
    u = ~u + 1;
  }
  return simd_uint64_to_string(u, out_buffer);
}

#else
// as written above, the same functions for M1 are just implemented without simd
// todo anyone who wants to practice some low-level magic — welcome to implement a proper SIMD form with ARM intrinsics
#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

template<size_t S, typename T>
inline int simd_value_to_string(char *out_buffer, const char *format, T value) noexcept {
  std::array<char, S> buffer{};
  int n = snprintf(buffer.data(), S, format, value);
  memcpy(out_buffer, buffer.data(), n);
  return n;
}

char *simd_uint32_to_string(uint32_t value, char *out_buffer) noexcept {
  return out_buffer + simd_value_to_string<11>(out_buffer, "%u", value);
  ;
}

char *simd_int32_to_string(int32_t value, char *out_buffer) noexcept {
  return out_buffer + simd_value_to_string<12>(out_buffer, "%d", value);
}

char *simd_uint64_to_string(uint64_t value, char *out_buffer) {
  return out_buffer + simd_value_to_string<21>(out_buffer, "%" PRIu64, value);
}

char *simd_int64_to_string(int64_t value, char *out_buffer) {
  return out_buffer + simd_value_to_string<21>(out_buffer, "%" PRId64, value);
}

#endif
