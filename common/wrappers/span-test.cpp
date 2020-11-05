// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <deque>
#include <gtest/gtest.h>

#include "common/wrappers/span.h"
#include "common/wrappers/string_view.h"

TEST(span_tests, empty) {
  {
    vk::span<int> s;
    ASSERT_EQ(s.size(), 0);
    ASSERT_EQ(s.begin(), s.end());
  }
  {
    vk::span<int> s(nullptr, 0);
    ASSERT_EQ(s.size(), 0);
    ASSERT_EQ(s.begin(), s.end());
  }
}

TEST(span_tests, from_iterator_and_size) {
  {
    std::vector<int> v{1, 2, 3};
    const std::vector<int> cv{1, 2, 3};
    vk::span<int> s{v.data() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = vk::span<int>{v.begin() + 1, 2};
    ASSERT_EQ(s[1], 3);
    static_assert(!std::is_constructible<vk::span<int>, decltype(v.cbegin()), size_t>{}, "can't drop const");
    static_assert(!std::is_constructible<vk::span<int>, decltype(cv.begin()), size_t>{}, "can't drop const");
    static_assert(!std::is_constructible<vk::span<int>, decltype(cv.cbegin()), size_t>{}, "can't drop const");
  }


  {
    std::vector<int> v{1, 2, 3};
    const std::vector<int> cv{1, 2, 3};
    vk::span<const int> s;
    s = {v.data() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = {v.begin() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = {v.cbegin() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = {cv.data() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = {cv.begin() + 1, 2};
    ASSERT_EQ(s[1], 3);
    s = {cv.cbegin() + 1, 2};
    ASSERT_EQ(s[1], 3);
  }

  static_assert(std::is_constructible<vk::span<int>, std::vector<int>::iterator, size_t>{}, "vector is contiguous");
  static_assert(!std::is_constructible<vk::span<int>, std::set<int>::iterator, size_t>{}, "set is not contiguous");
  static_assert(!std::is_constructible<vk::span<int>, std::deque<int>::iterator, size_t>{}, "set is not contiguous");
}

TEST(span_tests, from_two_iterators) {
  {
    std::vector<int> v{1, 2, 3};
    const std::vector<int> cv{1, 2, 3};
    vk::span<int> s{v.data() + 1, v.data() + 3};
    ASSERT_EQ(s[1], 3);
    s = vk::span<int>{v.begin() + 1, v.end()};
    ASSERT_EQ(s[1], 3);
    static_assert(!std::is_constructible<vk::span<int>, decltype(v.cbegin()), decltype(v.cend())>{}, "can't drop const");
    static_assert(!std::is_constructible<vk::span<int>, decltype(cv.begin()), decltype(cv.end())>{}, "can't drop const");
    static_assert(!std::is_constructible<vk::span<int>, decltype(cv.cbegin()), decltype(cv.cend())>{}, "can't drop const");
  }


  {
    std::vector<int> v{1, 2, 3};
    const std::vector<int> cv{1, 2, 3};
    vk::span<const int> s;
    s = {v.data() + 1, v.data() + 3};
    ASSERT_EQ(s[1], 3);
    s = {v.begin() + 1, v.end()};
    ASSERT_EQ(s[1], 3);
    s = {v.cbegin() + 1, v.cend()};
    ASSERT_EQ(s[1], 3);
    s = {cv.data() + 1, cv.data() + 3};
    ASSERT_EQ(s[1], 3);
    s = {cv.begin() + 1, cv.end()};
    ASSERT_EQ(s[1], 3);
    s = {cv.cbegin() + 1, cv.cend()};
    ASSERT_EQ(s[1], 3);
  }

  static_assert(std::is_constructible<vk::span<int>, std::vector<int>::iterator, std::vector<int>::iterator>{}, "vector is contiguous");
  static_assert(!std::is_constructible<vk::span<int>, std::set<int>::iterator, std::set<int>::iterator>{}, "set is not contiguous");
}

TEST(span_tests, from_container) {
  {
    std::vector<int> v{1, 2, 3};
    vk::span<int> s{v};
    ASSERT_EQ(s[2], 3);
    const std::vector<int> cv{1, 2, 3};
    static_assert(!std::is_constructible<vk::span<int>, decltype(cv)>{}, "can't drop const");
  }


  {
    std::vector<int> v{1, 2, 3};
    const std::vector<int> cv{1, 2, 3};
    vk::span<const int> s;
    s = v;
    ASSERT_EQ(s[2], 3);
    s = cv;
    ASSERT_EQ(s[2], 3);
  }

  static_assert(std::is_constructible<vk::span<int>, std::vector<int>&>{}, "vector is contiguous");
  static_assert(std::is_constructible<vk::span<char>, std::string&>{}, "string is contiguous");
  static_assert(std::is_constructible<vk::span<int>, std::array<int, 5>&>{}, "array is contiguous");
  static_assert(!std::is_constructible<vk::span<int>, std::set<int>&>{}, "set is not contiguous");
}

