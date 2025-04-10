// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <string>

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

TEST(string_view_tests, comparison) {
  vk::string_view sv1 = "some test string";
  vk::string_view sv2 = "some other string";
  auto s1 = std::string{sv1};
  auto s2 = std::string{sv2};
  EXPECT_EQ(sv1 < sv2, s1 < s2);
  EXPECT_EQ(sv2 < sv1, s2 < s1);
  EXPECT_EQ(sv1 == sv2, s1 == s2);

  EXPECT_EQ(sv1 < sv1, s1 < s1);
  EXPECT_EQ(sv1 == sv1, s1 == s1);
  EXPECT_EQ(sv1, vk::string_view{s1});

  sv1 = "aba";
  sv2 = "abacaba";
  EXPECT_LT(sv1, sv2);

  sv1 = "abc";
  EXPECT_LT(sv2, sv1);
}

TEST(string_view_tests, hash_consistency) {
  std::string s = "some test string";
  auto h1 = std::hash<std::string>()(s);
  auto h2 = vk::std_hash(vk::string_view(s));
  EXPECT_EQ(h1, h2);
}

TEST(string_view_tests, string_view) {
  vk::string_view sv1 = "test string";

  EXPECT_TRUE(sv1.starts_with("test"));
  EXPECT_TRUE(sv1.starts_with(std::string("test")));
  EXPECT_TRUE(sv1.starts_with(vk::string_view{"test"}));

  EXPECT_FALSE(sv1.starts_with("string"));
  EXPECT_FALSE(sv1.starts_with(" test"));
  EXPECT_FALSE(sv1.starts_with("test string "));
}

TEST(string_view_tests, find) {
  vk::string_view haystack = "abacabadabacaba";
  vk::string_view needle = "caba";
  EXPECT_EQ(haystack.find(needle), std::string{haystack}.find(std::string{needle}));
  EXPECT_EQ(haystack.find(needle, 5), std::string{haystack}.find(std::string{needle}, 5));
  needle = "not found";
  EXPECT_EQ(haystack.find(needle), std::string{haystack}.find(std::string{needle}));

  EXPECT_EQ(haystack.find('c'), std::string{haystack}.find('c'));
  EXPECT_EQ(haystack.find('z'), std::string{haystack}.find('z'));

  EXPECT_EQ(vk::string_view{}.find(""), std::string{}.find(""));
  EXPECT_EQ(vk::string_view{""}.find(""), std::string{""}.find(""));
  EXPECT_EQ(vk::string_view{"abc"}.find("", 1), std::string{"abc"}.find("", 1));
}

TEST(string_view_tests, rfind) {
  vk::string_view haystack = "abacabadabacaba";
  vk::string_view needle = "caba";
  EXPECT_EQ(haystack.rfind(needle), std::string{haystack}.rfind(std::string{needle}));
  EXPECT_EQ(haystack.rfind(needle, 5), std::string{haystack}.rfind(std::string{needle}, 5));
  needle = "not found";
  EXPECT_EQ(haystack.rfind(needle), std::string{haystack}.rfind(std::string{needle}));

  EXPECT_EQ(haystack.rfind('c'), std::string{haystack}.rfind('c'));
  EXPECT_EQ(haystack.rfind('z'), std::string{haystack}.rfind('z'));

  EXPECT_EQ(vk::string_view{}.rfind(""), std::string{}.rfind(""));
  EXPECT_EQ(vk::string_view{""}.rfind(""), std::string{""}.rfind(""));
  EXPECT_EQ(vk::string_view{"abc"}.rfind("", 1), std::string{"abc"}.rfind("", 1));
}

TEST(string_view_tests, find_first_of) {
  vk::string_view str = "[1, 2, 3]";
  EXPECT_EQ(str.find_first_of(","), std::string{str}.find_first_of(","));
  EXPECT_EQ(str.find_first_of("."), std::string{str}.find_first_of("."));
  EXPECT_EQ(str.find_first_of("[,"), std::string{str}.find_first_of("[,"));
  EXPECT_EQ(str.find_first_of("[,", 1), std::string{str}.find_first_of("[,", 1));

  EXPECT_EQ(vk::string_view{}.find_first_of(""), std::string{}.find_first_of(""));
}

TEST(string_view_tests, find_first_not_of) {
  vk::string_view str = "[1, 2, 3]";
  EXPECT_EQ(str.find_first_not_of(","), std::string{str}.find_first_not_of(","));
  EXPECT_EQ(str.find_first_not_of("."), std::string{str}.find_first_not_of("."));
  EXPECT_EQ(str.find_first_not_of("[,"), std::string{str}.find_first_not_of("[,"));
  EXPECT_EQ(str.find_first_not_of("[,", 1), std::string{str}.find_first_not_of("[,", 1));

  EXPECT_EQ(vk::string_view{}.find_first_not_of(""), std::string{}.find_first_not_of(""));
}

TEST(string_view_tests, remove_suffix) {
  vk::string_view str{"hello world"};
  str.remove_suffix(1);
  EXPECT_EQ(str, std::string{"hello worl"});

  str.remove_suffix(4);
  EXPECT_EQ(str, std::string{"hello "});

  str.remove_suffix(6);
  EXPECT_TRUE(str.empty());
}

TEST(string_view_tests, remove_prefix) {
  vk::string_view str{"hello world"};
  str.remove_prefix(1);
  EXPECT_EQ(str, std::string{"ello world"});

  str.remove_prefix(4);
  EXPECT_EQ(str, std::string{" world"});

  str.remove_prefix(6);
  EXPECT_TRUE(str.empty());
}
