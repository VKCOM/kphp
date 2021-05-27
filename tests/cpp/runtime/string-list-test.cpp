#include <gtest/gtest.h>

#include "runtime/string-list.h"

TEST(string_list_test, test_empty) {
  string_list l;
  ASSERT_TRUE(l.concat_and_get_string().empty());
}

TEST(string_list_test, test_build_and_get) {
  string_list l;

  ASSERT_TRUE(l.push_string("hello ", 6));
  ASSERT_TRUE(l.push_string("world", 5));
  ASSERT_TRUE(l.push_string(" foo", 4));
  ASSERT_TRUE(l.push_string(" ololol", 7));
  ASSERT_TRUE(l.push_string(" 12345", 6));

  auto s2 = l.concat_and_get_string();
  ASSERT_STREQ(s2.c_str(), "hello world foo ololol 12345");
  ASSERT_EQ(s2.size(), 28);
}

TEST(string_list_test, test_push_get_and_push) {
  string_list l;

  ASSERT_TRUE(l.push_string("hello ", 6));
  ASSERT_TRUE(l.push_string("world", 5));

  auto s1 = l.concat_and_get_string();
  ASSERT_STREQ(s1.c_str(), "hello world");
  ASSERT_EQ(s1.size(), 11);

  ASSERT_TRUE(l.push_string(" foo", 4));

  auto s2 = l.concat_and_get_string();
  ASSERT_STREQ(s2.c_str(), "hello world");
  ASSERT_EQ(s2.size(), 11);
}

TEST(string_list_test, test_push_and_reset) {
  string_list l;

  ASSERT_TRUE(l.push_string("hello ", 6));
  ASSERT_TRUE(l.push_string("world", 5));

  l.reset();
  ASSERT_TRUE(l.concat_and_get_string().empty());

  ASSERT_TRUE(l.push_string("hello ", 6));
  ASSERT_TRUE(l.push_string("world", 5));

  auto s1 = l.concat_and_get_string();
  ASSERT_STREQ(s1.c_str(), "hello world");
  ASSERT_EQ(s1.size(), 11);

  l.reset();
  ASSERT_TRUE(l.concat_and_get_string().empty());

  ASSERT_TRUE(l.push_string("foo ", 4));
  ASSERT_TRUE(l.push_string("bar baz", 7));

  auto s2 = l.concat_and_get_string();
  ASSERT_STREQ(s2.c_str(), "foo bar baz");
  ASSERT_EQ(s2.size(), 11);
}
