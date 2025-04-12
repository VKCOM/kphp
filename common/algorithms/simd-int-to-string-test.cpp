// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/simd-int-to-string.h"

#include <limits>
#include <string>

#include <array>
#include <gtest/gtest.h>

namespace {
std::string int32_to_string(int32_t x) {
  std::array<char, 11> buffer{};
  return {buffer.data(), simd_int32_to_string(x, buffer.data())};
}

std::string uint32_to_string(uint32_t x) {
  std::array<char, 10> buffer{};
  return {buffer.data(), simd_uint32_to_string(x, buffer.data())};
}

std::string int64_to_string(int64_t x) {
  std::array<char, 20> buffer{};
  return {buffer.data(), simd_int64_to_string(x, buffer.data())};
}

std::string uint64_to_string(uint64_t x) {
  std::array<char, 20> buffer{};
  return {buffer.data(), simd_uint64_to_string(x, buffer.data())};
}
} // namespace

TEST(simd_int_to_string, int32_type) {
  for (int32_t x : {0,
                    1,
                    7,
                    32,
                    100,
                    1234,
                    48662,
                    987421,
                    2312421,
                    12349480,
                    908374261,
                    2071623429,
                    std::numeric_limits<int32_t>::max(),
                    -1,
                    -6,
                    -21,
                    -231,
                    -5421,
                    -76542,
                    -421343,
                    -4213123,
                    -80912429,
                    -942183212,
                    -2023124282,
                    std::numeric_limits<int32_t>::min()}) {
    ASSERT_EQ(int32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, int32_type_bruteforce) {
  for (int32_t x = -500000; x != 500000; ++x) {
    ASSERT_EQ(int32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, uint32_type) {
  for (uint32_t x : {0u, 1u, 7u, 100u, 1234u, 48662u, 987421u, 2312421u, 12349480u, 908374261u, 2071623429u, std::numeric_limits<uint32_t>::max()}) {
    ASSERT_EQ(uint32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, uint32_type_bruteforce) {
  for (uint32_t x = 0; x != 1000000; ++x) {
    ASSERT_EQ(uint32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, int64_type) {
  for (long long x : {0ll,
                      1ll,
                      7ll,
                      32ll,
                      100ll,
                      1234ll,
                      48662ll,
                      987421ll,
                      2312421ll,
                      12349480ll,
                      908374261ll,
                      2071623429ll,
                      21314239376ll,
                      386152830271ll,
                      8261523728471ll,
                      47236161254237ll,
                      124123483625152ll,
                      9290374629102472ll,
                      48271629402920182ll,
                      402927465129305827ll,
                      9130182950294756281ll,
                      std::numeric_limits<long long>::max(),
                      -1ll,
                      -6ll,
                      -21ll,
                      -231ll,
                      -5421ll,
                      -76542ll,
                      -421343ll,
                      -4213123ll,
                      -80912429ll,
                      -942183212ll,
                      -2023124282ll,
                      -41231242354ll,
                      -902391249248ll,
                      -9478262371824ll,
                      -82732163524627ll,
                      -502918273746261ll,
                      -3948271626535251ll,
                      -94827172364525150ll,
                      -112329324029120094ll,
                      -4928182428546592010ll,
                      std::numeric_limits<long long>::min()}) {
    ASSERT_EQ(int64_to_string(static_cast<int64_t>(x)), std::to_string(x));
  }
}

TEST(simd_int_to_string, uint64_type) {
  for (unsigned long long x : {0ull,
                               1ull,
                               7ull,
                               32ull,
                               100ull,
                               1234ull,
                               48662ull,
                               987421ull,
                               2312421ull,
                               12349480ull,
                               908374261ull,
                               2071623429ull,
                               21314239376ull,
                               386152830271ull,
                               8261523728471ull,
                               47236161254237ull,
                               124123483625152ull,
                               9290374629102472ull,
                               48271629402920182ull,
                               402927465129305827ull,
                               9130182950294756281ull,
                               std::numeric_limits<unsigned long long>::max()}) {
    ASSERT_EQ(uint64_to_string(static_cast<uint64_t>(x)), std::to_string(x));
  }
}
