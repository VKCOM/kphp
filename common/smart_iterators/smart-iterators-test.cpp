// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "common/smart_iterators/filter_iterator.h"
#include "common/smart_iterators/take_while_iterator.h"
#include "common/smart_iterators/transform_iterator.h"
#include "common/wrappers/iterator_range.h"

using namespace ::testing;

TEST(filter_iterator, filter_odd) {
  std::vector<int> v{1, 2, 3, 4};
  auto odd_numbers = vk::make_filter_iterator_range([](int x) { return x % 2 != 0; }, v.begin(), v.end());
  EXPECT_THAT(odd_numbers, ElementsAre(1, 3));
}

static bool is_odd(int x) {
  return x % 2 != 0;
}
TEST(filter_iterator, filter_odd_with_function) {
  std::vector<int> v{1, 2, 3, 4};
  auto odd_numbers = vk::make_filter_iterator_range(is_odd, v.begin(), v.end());
  EXPECT_THAT(odd_numbers, ElementsAre(1, 3));
}

TEST(transform_iterator, square_strings) {
  std::vector<int> v{1, 2, 3, 4};
  auto square_strings = vk::make_transform_iterator_range([](int x) { return std::to_string(x * x); }, v.begin(), v.end());
  EXPECT_THAT(square_strings, ElementsAre("1", "4", "9", "16"));
}

static auto square_str(int x) {
  return std::to_string(x * x);
}
TEST(transform_iterator, square_strings_function) {
  std::vector<int> v{1, 2, 3, 4};
  auto square_strings = vk::make_transform_iterator_range(square_str, v.begin(), v.end());
  EXPECT_THAT(square_strings, ElementsAre("1", "4", "9", "16"));
}

TEST(take_while_iterator, less_10) {
  std::vector<int> v{1, 2, 3, 10, 15, 20, 4, 5};
  auto first_less_10_numbers = vk::make_take_while_iterator_range([](int x) { return x < 10; }, v.begin(), v.end());
  EXPECT_THAT(first_less_10_numbers, ElementsAre(1, 2, 3));
}

static bool less_10(int x) {
  return x < 10;
}
TEST(take_while_iterator, less_10_function) {
  std::vector<int> v{1, 2, 3, 10, 15, 20, 4, 5};
  auto first_less_10_numbers = vk::make_take_while_iterator_range(less_10, v.begin(), v.end());
  EXPECT_THAT(first_less_10_numbers, ElementsAre(1, 2, 3));
}
