// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <random>
#include <vector>

#include "common/algorithms/adjacent_find.h"
#include "common/algorithms/lower_bound.h"
#include "common/algorithms/mismatch.h"
#include "common/algorithms/sort.h"
#include "common/algorithms/stable_sort.h"
#include "common/algorithms/upper_bound.h"
#include "common/kprintf.h"

TEST(algorithms_projections_test, sort) {
  std::vector<std::pair<int, int>> rng = {{1, -2}, {2, -3}, {3, -4}};
  auto rng_copy = rng;
  vk::sort(rng, &std::pair<int, int>::second);
  std::reverse(rng.begin(), rng.end());
  EXPECT_EQ(rng, rng_copy);
  vk::sort(rng, &std::pair<int, int>::second, std::greater<int>());
  EXPECT_EQ(rng, rng_copy);
}

TEST(algorithms_projections_test, stable_sort) {
  std::vector<std::pair<int, int>> rng = {{1, -2}, {2, -3}, {3, -3}};
  auto rng_copy = rng;
  vk::stable_sort(rng, &std::pair<int, int>::second);
  std::reverse(rng.begin(), rng.end());
  EXPECT_NE(rng, rng_copy);
  EXPECT_EQ(rng.back(), std::make_pair(2, -3));
  std::swap(rng[1], rng[2]);
  EXPECT_EQ(rng, rng_copy);
  vk::stable_sort(rng, &std::pair<int, int>::second, std::greater<int>());
  EXPECT_EQ(rng, rng_copy);
}

TEST(algorithms_projections_test, lower_bound) {
  std::vector<std::pair<int, int>> rng = {{1, -2}, {2, -3}, {3, -4}};
  auto it = vk::lower_bound(rng, -3, &std::pair<int, int>::second, std::greater<int>());
  EXPECT_NE(it, rng.end());
  EXPECT_EQ(it->second, -3);
}

TEST(algorithms_projections_test, upper_bound) {
  std::vector<std::pair<int, int>> rng = {{1, -2}, {2, -3}, {3, -4}};
  auto it = vk::upper_bound(rng, -3, &std::pair<int, int>::second, std::greater<int>());
  EXPECT_NE(it, rng.end());
  EXPECT_EQ(it->second, -4);
}

TEST(algorithms_projections_test, sort_lower_bound_cooperation) {
  std::vector<std::pair<int, int>> rng;
  std::mt19937 rnd(std::random_device{}());
  for (int i = 0; i < 1000; i++) {
    int x = rnd();
    int y = rnd();
    rng.emplace_back(x, y);
  }
  constexpr auto proj = &std::pair<int, int>::second;
  vk::sort(rng, proj);
  const int needle = rnd();

  EXPECT_EQ(vk::lower_bound(rng, needle, proj), std::find_if(rng.begin(), rng.end(), [needle](std::pair<int, int> p){ return p.second >= needle; }));
}

TEST(algorithms_projections_test, adjacent_find) {
  std::vector<std::pair<int, int>> rng = {{1, -2}, {2, -2}, {3, -4}};
  auto it = vk::adjacent_find(rng, &std::pair<int, int>::second);
  EXPECT_NE(it, rng.end());
  EXPECT_EQ(it->second, -2);
}

TEST(algorithms_projections_test, mismatch) {
  std::array<int, 4> rng1 = {1, 3, 3, 7};
  std::array<int, 4> rng2 = {1, 4, 8, 8};
  auto it = vk::mismatch(rng1, rng2);
  EXPECT_EQ(it.first, rng1.begin() + 1);
  EXPECT_EQ(it.second, rng2.begin() + 1);
}
