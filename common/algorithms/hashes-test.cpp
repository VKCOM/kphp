// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <vector>
#include <iterator>

#include "common/algorithms/hashes.h"

TEST(algorithms_hashes_test, vector) {
  const std::vector<int> rng = {1, 2, 3};
  auto h2 = vk::hash_range(rng);
  auto h1 = vk::hash_range(rng.begin(), rng.end());
  EXPECT_EQ(h1, h2);
}

TEST(algorithms_hashes_test, array) {
  const int rng[] = {1, 2, 3};
  auto h2 = vk::hash_range(rng);
  auto h1 = vk::hash_range(rng, rng + 3);
  EXPECT_EQ(h1, h2);
}

TEST(algorithms_hashes_test, custom_hasher) {
  const std::string s = "abacaba";
  auto hasher = [](char c) { return ~c; };
  auto h1 = vk::hash_range(s, hasher);
  auto h2 = vk::hash_range(s.begin(), s.end(), hasher);
  EXPECT_EQ(h1, h2);
}
