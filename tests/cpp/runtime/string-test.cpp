#include <gtest/gtest.h>

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"

TEST(string_test, test_empty) {
  string empty_str;
  ASSERT_TRUE(empty_str.empty());

  empty_str.append("");
  ASSERT_TRUE(empty_str.empty());

  empty_str.assign("");
  ASSERT_TRUE(empty_str.empty());

  empty_str = string{};
  ASSERT_TRUE(empty_str.empty());

  empty_str = string{""};
  ASSERT_TRUE(empty_str.empty());

  empty_str.assign("hello");
  ASSERT_FALSE(empty_str.empty());
}

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

TEST(string_test, test_ends_with) {
  string empty_str{""};
  ASSERT_TRUE(empty_str.ends_with(string{""}));
  ASSERT_FALSE(empty_str.ends_with(string{"hello world"}));

  string str{"hello world"};
  ASSERT_TRUE(str.ends_with(string{""}));
  ASSERT_TRUE(str.ends_with(string{"d"}));
  ASSERT_TRUE(str.ends_with(string{"world"}));
  ASSERT_TRUE(str.ends_with(string{"hello world"}));

  ASSERT_FALSE(str.ends_with(string{"x"}));
  ASSERT_FALSE(str.ends_with(string{"hello"}));
  ASSERT_FALSE(str.ends_with(string{"world!"}));
  ASSERT_FALSE(str.starts_with(string{"hello world!"}));
}

TEST(string_test, test_contains) {
  string empty_str{""};
  ASSERT_TRUE(empty_str.contains(string{""}));
  ASSERT_FALSE(empty_str.contains(string{"a"}));

  string str{"hello world"};
  ASSERT_TRUE(str.contains(string{"hello"}));
  ASSERT_TRUE(str.contains(string{"world"}));
  ASSERT_TRUE(str.contains(string{"orld"}));
  ASSERT_TRUE(str.contains(string{"o w"}));
  ASSERT_TRUE(str.contains(string{"d"}));
  ASSERT_TRUE(str.contains(string{""}));

  ASSERT_FALSE(str.contains(string{"hEllo"}));
  ASSERT_FALSE(str.contains(string{"o W"}));

  ASSERT_FALSE(str.contains(string{"hello world!"}));
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

TEST(string_test, test_hex_to_int) {
  for (size_t c = 0; c != 256; ++c) {
    if (vk::none_of_equal(c,
                          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                          'a', 'b', 'c', 'd', 'e', 'f',
                          'A', 'B', 'C', 'D', 'E', 'F')) {
      ASSERT_EQ(hex_to_int(static_cast<char>(c)), 16);
    }
  }
  ASSERT_EQ(hex_to_int('0'), 0);
  ASSERT_EQ(hex_to_int('1'), 1);
  ASSERT_EQ(hex_to_int('2'), 2);
  ASSERT_EQ(hex_to_int('3'), 3);
  ASSERT_EQ(hex_to_int('4'), 4);
  ASSERT_EQ(hex_to_int('5'), 5);
  ASSERT_EQ(hex_to_int('6'), 6);
  ASSERT_EQ(hex_to_int('7'), 7);
  ASSERT_EQ(hex_to_int('8'), 8);
  ASSERT_EQ(hex_to_int('9'), 9);

  ASSERT_EQ(hex_to_int('a'), 10);
  ASSERT_EQ(hex_to_int('b'), 11);
  ASSERT_EQ(hex_to_int('c'), 12);
  ASSERT_EQ(hex_to_int('d'), 13);
  ASSERT_EQ(hex_to_int('e'), 14);
  ASSERT_EQ(hex_to_int('f'), 15);

  ASSERT_EQ(hex_to_int('A'), 10);
  ASSERT_EQ(hex_to_int('B'), 11);
  ASSERT_EQ(hex_to_int('C'), 12);
  ASSERT_EQ(hex_to_int('D'), 13);
  ASSERT_EQ(hex_to_int('E'), 14);
  ASSERT_EQ(hex_to_int('F'), 15);
}