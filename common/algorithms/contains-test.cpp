// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include <string>
#include <vector>

#include "common/algorithms/contains.h"
#include "common/wrappers/string_view.h"

TEST(algorithms_contains, map) {
  std::map<int, int> m{{1, 2}, {3, 4}, {7, 8}};
  ASSERT_TRUE(vk::contains(m, 1));
  ASSERT_TRUE(vk::contains(m, 3));
  ASSERT_TRUE(vk::contains(m, 7));
  ASSERT_FALSE(vk::contains(m, 2));
}

TEST(algorithms_contains, vector) {
  std::vector<int> v{1, 3, 7};
  ASSERT_TRUE(vk::contains(v, 1));
  ASSERT_TRUE(vk::contains(v, 3));
  ASSERT_TRUE(vk::contains(v, 7));
  ASSERT_FALSE(vk::contains(v, 2));
}

TEST(algorithms_contains, c_array) {
  int x[] = {1, 3};
  ASSERT_TRUE(vk::contains(x, 1));
  ASSERT_TRUE(vk::contains(x, 3));
  ASSERT_FALSE(vk::contains(x, 2));
}

TEST(algorithms_contains, std_string) {
  std::string s{"12345678"};
  ASSERT_TRUE(vk::contains(s, "23"));
  ASSERT_TRUE(vk::contains(s, '1'));
  ASSERT_FALSE(vk::contains(s, "92"));
}

TEST(algorithms_contains, vk_string_view) {
  vk::string_view s{"12345678"};
  ASSERT_TRUE(vk::contains(s, "23"));
  ASSERT_TRUE(vk::contains(s, '1'));
  ASSERT_FALSE(vk::contains(s, "92"));
}

TEST(algorithms_contains, find_was_called) {
  struct string_view_mock : vk::string_view {
    using vk::string_view::npos;
    MOCK_CONST_METHOD1(find, size_t (char));
    MOCK_CONST_METHOD1(find, size_t (const char *));
  };
  using ::testing::Return;

  string_view_mock w;
  EXPECT_CALL(w, find('1'))
    .Times(1)
    .WillOnce(Return(vk::string_view::npos));

  EXPECT_CALL(w, find("12"))
    .Times(1)
    .WillOnce(Return(vk::string_view::npos));

  ASSERT_FALSE(vk::contains(w, '1'));
  ASSERT_FALSE(vk::contains(w, "12"));
}
