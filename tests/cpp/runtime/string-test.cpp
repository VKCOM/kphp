#include <gtest/gtest.h>

#include "runtime/kphp_core.h"

TEST(string_test, test_starts_with) {
  string empty_str{""};
  ASSERT_TRUE(empty_str.starts_with(string{""}));
  ASSERT_FALSE(empty_str.starts_with(string{"hello world"}));

  string str{"hello world"};
  ASSERT_TRUE(str.starts_with(string{""}));
  ASSERT_TRUE(str.starts_with(string{"h"}));
  ASSERT_TRUE(str.starts_with(string{"hell"}));
  ASSERT_TRUE(str.starts_with(string{"hello world"}));

  ASSERT_FALSE(str.starts_with(string{"x"}));
  ASSERT_FALSE(str.starts_with(string{"hi"}));
  ASSERT_FALSE(str.starts_with(string{"hello world!"}));
}

TEST(string_test, test_make_const_string_on_memory) {
  char mem[1024];

  auto s = string::make_const_string_on_memory("hello", 5, mem, sizeof(mem));
  ASSERT_TRUE(s.is_reference_counter(ExtraRefCnt::for_global_const));
  ASSERT_EQ(s, string{"hello"});
}

TEST(string_test, test_copy_and_make_not_shared) {
  const string str1{"hello world"};
  const string str2 = str1;
  ASSERT_EQ(str1.c_str(), str2.c_str());

  ASSERT_EQ(str1.get_reference_counter(), 2);
  ASSERT_EQ(str2.get_reference_counter(), 2);

  const string str3 = str1.copy_and_make_not_shared();

  ASSERT_EQ(str1.c_str(), str2.c_str());
  ASSERT_EQ(str1.get_reference_counter(), 2);
  ASSERT_EQ(str2.get_reference_counter(), 2);

  ASSERT_NE(str1.c_str(), str3.c_str());
  ASSERT_EQ(str3.get_reference_counter(), 1);
}
