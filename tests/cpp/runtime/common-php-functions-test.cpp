#include <gtest/gtest.h>

#include "common/php-functions.h"

bool php_is_int_wrapper(const std::string &s) {
  return php_is_int(s.c_str(), s.size());
}

bool php_try_to_int_wrapper(const std::string &s, int64_t &value) {
  return php_try_to_int(s.c_str(), s.size(), &value);
}

TEST(test_php_is_integer, non_integer_strings) {
  ASSERT_FALSE(php_is_int_wrapper(""));
  ASSERT_FALSE(php_is_int_wrapper(" "));
  ASSERT_FALSE(php_is_int_wrapper("xx"));
  ASSERT_FALSE(php_is_int_wrapper("abs"));
  ASSERT_FALSE(php_is_int_wrapper("123 "));
  ASSERT_FALSE(php_is_int_wrapper("123 23"));
  ASSERT_FALSE(php_is_int_wrapper("123.23"));
  ASSERT_FALSE(php_is_int_wrapper("-0"));
  ASSERT_FALSE(php_is_int_wrapper("+0"));
  ASSERT_FALSE(php_is_int_wrapper("05882"));
  ASSERT_FALSE(php_is_int_wrapper("-0421312"));
  ASSERT_FALSE(php_is_int_wrapper("+0421314"));
}

TEST(test_php_is_integer, integer_strings) {
  ASSERT_TRUE(php_is_int_wrapper("1"));
  ASSERT_TRUE(php_is_int_wrapper("-1"));
  ASSERT_TRUE(php_is_int_wrapper("+1"));
  ASSERT_TRUE(php_is_int_wrapper("0"));
  ASSERT_TRUE(php_is_int_wrapper("48948"));
  ASSERT_TRUE(php_is_int_wrapper("+848189148"));
  ASSERT_TRUE(php_is_int_wrapper("-4214241"));
  ASSERT_TRUE(php_is_int_wrapper("-2147483648"));
  ASSERT_TRUE(php_is_int_wrapper("2147483647"));
  ASSERT_TRUE(php_is_int_wrapper("56398164621042224"));
  ASSERT_TRUE(php_is_int_wrapper("3333333333"));
  ASSERT_TRUE(php_is_int_wrapper("2147483648"));
  ASSERT_TRUE(php_is_int_wrapper("3147483647"));
  ASSERT_TRUE(php_is_int_wrapper("-9572623627881742"));
  ASSERT_TRUE(php_is_int_wrapper("-4444444444"));
  ASSERT_TRUE(php_is_int_wrapper("-2147483649"));
  ASSERT_TRUE(php_is_int_wrapper("983014289185212"));
  ASSERT_TRUE(php_is_int_wrapper("+894521763729189"));
  ASSERT_TRUE(php_is_int_wrapper("-572187372942817"));
  ASSERT_TRUE(php_is_int_wrapper("9189237823724728912"));
  ASSERT_TRUE(php_is_int_wrapper("9223372036854775807"));
  ASSERT_TRUE(php_is_int_wrapper("-9218237236124482742"));
  ASSERT_TRUE(php_is_int_wrapper("-9223372036854775808"));
}

TEST(test_php_is_integer, long_strings) {
  ASSERT_FALSE(php_is_int_wrapper("984894179184498189851"));
  ASSERT_FALSE(php_is_int_wrapper("9223372036854775808"));
  ASSERT_FALSE(php_is_int_wrapper("-784894841981984984891498"));
  ASSERT_FALSE(php_is_int_wrapper("-9223372036854775809"));
}

TEST(test_php_try_to_integer, non_integer_strings) {
  int64_t x = 0;
  ASSERT_FALSE(php_try_to_int_wrapper("", x));
  ASSERT_FALSE(php_try_to_int_wrapper(" ", x));
  ASSERT_FALSE(php_try_to_int_wrapper("xx", x));
  ASSERT_FALSE(php_try_to_int_wrapper("abs", x));
  ASSERT_FALSE(php_try_to_int_wrapper("123 ", x));
  ASSERT_FALSE(php_try_to_int_wrapper("123 23", x));
  ASSERT_FALSE(php_try_to_int_wrapper("123.23", x));
  ASSERT_FALSE(php_try_to_int_wrapper("-0", x));
  ASSERT_FALSE(php_try_to_int_wrapper("+0", x));
  ASSERT_FALSE(php_try_to_int_wrapper("05882", x));
  ASSERT_FALSE(php_try_to_int_wrapper("-0421312", x));
  ASSERT_FALSE(php_try_to_int_wrapper("+0421314", x));
  ASSERT_FALSE(php_try_to_int_wrapper("+123", x));
}

TEST(test_php_try_to_integer, integer_strings) {
  int64_t x = 0;

  ASSERT_TRUE(php_try_to_int_wrapper("1", x));
  ASSERT_EQ(x, 1);

  ASSERT_TRUE(php_try_to_int_wrapper("-1", x));
  ASSERT_EQ(x, -1);

  ASSERT_TRUE(php_try_to_int_wrapper("0", x));
  ASSERT_EQ(x, 0);

  ASSERT_TRUE(php_try_to_int_wrapper("48948", x));
  ASSERT_EQ(x, 48948);

  ASSERT_TRUE(php_try_to_int_wrapper("-4214241", x));
  ASSERT_EQ(x, -4214241);

  ASSERT_TRUE(php_try_to_int_wrapper("2147483647", x));
  ASSERT_EQ(x, 2147483647);

  ASSERT_TRUE(php_try_to_int_wrapper("-2147483648", x));
  ASSERT_EQ(x, -2147483648);

  ASSERT_TRUE(php_try_to_int_wrapper("56398164621042224", x));
  ASSERT_EQ(x, 56398164621042224L);

  ASSERT_TRUE(php_try_to_int_wrapper("3333333333", x));
  ASSERT_EQ(x, 3333333333L);

  ASSERT_TRUE(php_try_to_int_wrapper("2147483648", x));
  ASSERT_EQ(x, 2147483648L);

  ASSERT_TRUE(php_try_to_int_wrapper("3147483647", x));
  ASSERT_EQ(x, 3147483647L);

  ASSERT_TRUE(php_try_to_int_wrapper("-9572623627881742", x));
  ASSERT_EQ(x, -9572623627881742L);

  ASSERT_TRUE(php_try_to_int_wrapper("-4444444444", x));
  ASSERT_EQ(x, -4444444444L);

  ASSERT_TRUE(php_try_to_int_wrapper("-2147483649", x));
  ASSERT_EQ(x, -2147483649L);

  ASSERT_TRUE(php_try_to_int_wrapper("983014289185212", x));
  ASSERT_EQ(x, 983014289185212L);

  ASSERT_TRUE(php_try_to_int_wrapper("-572187372942817", x));
  ASSERT_EQ(x, -572187372942817L);

  ASSERT_TRUE(php_try_to_int_wrapper("9189237823724728912", x));
  ASSERT_EQ(x, 9189237823724728912L);

  ASSERT_TRUE(php_try_to_int_wrapper("9223372036854775807", x));
  ASSERT_EQ(x, 9223372036854775807L);

  ASSERT_TRUE(php_try_to_int_wrapper("-9218237236124482742", x));
  ASSERT_EQ(x, -9218237236124482742L);

  ASSERT_TRUE(php_try_to_int_wrapper("-9223372036854775808", x));
  ASSERT_EQ(x, std::numeric_limits<int64_t>::min());
}

TEST(test_php_try_to_integer, long_strings) {
  int64_t x = 0;
  ASSERT_FALSE(php_try_to_int_wrapper("984894179184498189851", x));
  ASSERT_FALSE(php_try_to_int_wrapper("9223372036854775808", x));
  ASSERT_FALSE(php_try_to_int_wrapper("-784894841981984984891498", x));
  ASSERT_FALSE(php_try_to_int_wrapper("-9223372036854775809", x));
}
