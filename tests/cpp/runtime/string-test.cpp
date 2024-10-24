#include <gtest/gtest.h>

#include "runtime-common/core/runtime-core.h"
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

TEST(string_test, test_to_int) {
  struct TestCase {
    std::string s;
    string::size_type l;
    int64_t want;
  };
  std::vector<TestCase> tests = {
    {" ", 0, 0},
    {" ", 1, 0},
    {" -123", 1, 0},
    {" -123", 2, 0},
    {" -123", 3, -1},
    {" -123", 4, -12},
    {" -123", 5, -123},
    {"  -123", 5, -12},
    {"   -123", 5, -1},
    {"    -123", 5, 0},
    {"     -123", 5, 0},
    {" +123", 1, 0},
    {" +123", 2, 0},
    {" +123", 3, 1},
    {" +123", 4, 12},
    {" +123", 5, 123},
    {"  +123", 5, 12},
    {"   +123", 5, 1},
    {"    +123", 5, 0},
    {"     +123", 5, 0},
    {" 123 ", 1, 0},
    {" 123 ", 2, 1},
    {" 123 ", 3, 12},
    {" 123 ", 4, 123},
    {" 123 ", 5, 123},
    {"  123 ", 5, 123},
    {"   123 ", 5, 12},
    {"    123 ", 5, 1},
    {"     123 ", 5, 0},
  };
  for (const auto &test : tests) {
    int64_t have = string::to_int(test.s.c_str(), test.l);
    ASSERT_EQ(have, test.want) << "s=" << test.s << ", l=" << test.l;
  }
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