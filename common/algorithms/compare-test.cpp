// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <iterator>
#include <random>
#include <vector>

#include "common/algorithms/compare.h"

TEST(algorithms_compare, same) {
  const std::vector<int> rng = {1, 2, 3};
  auto c = vk::three_way_lexicographical_compare(rng, rng);
  EXPECT_EQ(0, c);
}

TEST(algorithms_compare, different_ranges) {
  const std::vector<int> rng1 = {1, 2, 3};
  const int rng2[] = {1, 2, 3};
  auto c = vk::three_way_lexicographical_compare(rng1, rng2);
  EXPECT_EQ(0, c);
}

TEST(algorithms_compare, different_length) {
  std::vector<int> rng1 = {1, 2, 3, 4};
  const int rng2[] = {1, 2, 3};
  auto c = vk::three_way_lexicographical_compare(rng1, rng2);
  EXPECT_GT(c, 0);
  rng1.resize(2);
  c = vk::three_way_lexicographical_compare(rng1, rng2);
  EXPECT_LT(c, 0);
}

TEST(algorithms_compare, random_ranges) {
  std::default_random_engine random;
  size_t l1 = random() % 100 + 10;
  size_t l2 = random() % 100 + 10;
  std::vector<uint8_t> v1(l1);
  std::vector<uint8_t> v2(l2);
  for (auto& c : v1) {
    c = random();
  }
  for (auto& c : v2) {
    c = random();
  }

  auto result = vk::three_way_lexicographical_compare(v1, v2);

  EXPECT_TRUE(!(v1 < v2) || result < 0);
  EXPECT_TRUE(!(v1 == v2) || result == 0);
  EXPECT_TRUE(!(v1 > v2) || result > 0);
}
