// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/simd-int-to-string.h"

#include <limits>
#include <string>

#include <gtest/gtest.h>

namespace {
std::string int32_to_string(int32_t x) {
  char buffer[11];
  return std::string{buffer, simd_int32_to_string(x, buffer)};
}

std::string uint32_to_string(uint32_t x) {
  char buffer[10];
  return std::string{buffer, simd_uint32_to_string(x, buffer)};
}

std::string int64_to_string(int64_t x) {
  char buffer[20];
  return std::string{buffer, simd_int64_to_string(x, buffer)};
}

std::string uint64_to_string(uint64_t x) {
  char buffer[20];
  return std::string{buffer, simd_uint64_to_string(x, buffer)};
}
}

TEST(simd_int_to_string, int32_type) {
  for (int32_t x : {0, 1, 7,
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
                    -1, -6,
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
  for (uint32_t x : {0u, 1u, 7u,
                     100u,
                     1234u,
                     48662u,
                     987421u,
                     2312421u,
                     12349480u,
                     908374261u,
                     2071623429u,
                     std::numeric_limits<uint32_t>::max()}) {
    ASSERT_EQ(uint32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, uint32_type_bruteforce) {
  for (uint32_t x = 0; x != 1000000; ++x) {
    ASSERT_EQ(uint32_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, int64_type) {
  for (int64_t x : {0l, 1l, 7l,
                    32l,
                    100l,
                    1234l,
                    48662l,
                    987421l,
                    2312421l,
                    12349480l,
                    908374261l,
                    2071623429l,
                    21314239376l,
                    386152830271l,
                    8261523728471l,
                    47236161254237l,
                    124123483625152l,
                    9290374629102472l,
                    48271629402920182l,
                    402927465129305827l,
                    9130182950294756281l,
                    std::numeric_limits<int64_t>::max(),
                    -1l, -6l,
                    -21l,
                    -231l,
                    -5421l,
                    -76542l,
                    -421343l,
                    -4213123l,
                    -80912429l,
                    -942183212l,
                    -2023124282l,
                    -41231242354l,
                    -902391249248l,
                    -9478262371824l,
                    -82732163524627l,
                    -502918273746261l,
                    -3948271626535251l,
                    -94827172364525150l,
                    -112329324029120094l,
                    -4928182428546592010l,
                    std::numeric_limits<int64_t>::min()}) {
    ASSERT_EQ(int64_to_string(x), std::to_string(x));
  }
}

TEST(simd_int_to_string, uint64_type) {
  for (uint64_t x : {0ul, 1ul, 7ul,
                     32ul,
                     100ul,
                     1234ul,
                     48662ul,
                     987421ul,
                     2312421ul,
                     12349480ul,
                     908374261ul,
                     2071623429ul,
                     21314239376ul,
                     386152830271ul,
                     8261523728471ul,
                     47236161254237ul,
                     124123483625152ul,
                     9290374629102472ul,
                     48271629402920182ul,
                     402927465129305827ul,
                     9130182950294756281ul,
                     std::numeric_limits<uint64_t>::max()}) {
    ASSERT_EQ(uint64_to_string(x), std::to_string(x));
  }
}
