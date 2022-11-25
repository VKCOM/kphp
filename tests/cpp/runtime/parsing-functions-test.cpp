#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include "runtime/parsing_functions.h"

/*
 * Tests for helper functions
 */

TEST(parsing_functions_test, test_clear_extra_spaces) {
  ASSERT_STREQ(
    string("This is string without extra spaces").c_str(),
    clearExtraSpaces(string("This           is     string   without     extra spaces")).c_str()
  );
}

TEST(parsing_functions_test, test_clear_quotes) {
  ASSERT_STREQ(
    string("Single quotes string").c_str(),
    clearQuotes(string("\'Single quotes string\'")).c_str()
  );
  ASSERT_STREQ(
    string("Double quotes string").c_str(),
    clearQuotes(string("\"Double quotes string\"")).c_str()
  );
}

TEST(parsing_functions_test, test_tolower) {
  ASSERT_STREQ(string("on").c_str(), tolower(string("On")).c_str());
  ASSERT_STREQ(string("off").c_str(), tolower(string("Off")).c_str());
  ASSERT_STREQ(string("true").c_str(), tolower(string("TRUE")).c_str());
  ASSERT_STREQ(string("false").c_str(), tolower(string("FALSE")).c_str());
}

/*
 * Tests for ini parsing functions
 */

TEST(parsing_functions_test, test_get_ini_var) {
  ASSERT_STREQ(string("foo_var").c_str(), get_ini_var(string("foo_var=123")).c_str());
  ASSERT_STREQ(string("bar_var").c_str(), get_ini_var(string("bar_var=123")).c_str());
}

TEST(parsing_functions_test, test_get_ini_val) {
    ASSERT_STREQ(string("hello world").c_str(), get_ini_val(string("str=hello world")).c_str());
    ASSERT_STREQ(string("1207").c_str(), get_ini_val(string("int=1207")).c_str());
    ASSERT_STREQ(string("3.14").c_str(), get_ini_val(string("pi=3.14")).c_str());
    ASSERT_STREQ(string("127.0.0.1").c_str(), get_ini_val(string("addr=127.0.0.1")).c_str());
}

TEST(parsing_functions_test, test_parse_ini_string_empty) {
  array<mixed> res = f$parse_ini_string(string(""));
  ASSERT_TRUE(res.empty());
}

TEST(parsing_functions_test, test_parse_ini_string_process_section_false) {
  array<mixed> res(array_size(0, 0, true));

  res = f$parse_ini_string(string("[one] hello=world world=hello [two] a=a b=b"));
  ASSERT_EQ(res.size().string_size, 4);

  res = f$parse_ini_string(string("hello=world world=hello a=a b=b"));
  ASSERT_EQ(res.size().string_size, 4);
}

TEST(parsing_functions_test, test_parse_ini_string_process_section_true) {
  array<mixed> res(array_size(0, 0, true));

  res = f$parse_ini_string(string("[one] [two]"), true);
  ASSERT_EQ(res.size().string_size, 2);
  ASSERT_TRUE(res.has_key(string("one")));
  ASSERT_TRUE(res.has_key(string("two")));

  res = f$parse_ini_string(string("[one] hello=world world=hello [two] a=a b=b"), true);

  ASSERT_EQ(res.size().string_size, 2);
  ASSERT_TRUE(res.has_key(string("one")));
  ASSERT_TRUE(res.get_value(string("one")).is_array());
  ASSERT_TRUE(res.get_value(string("one")).to_array().has_key(string("hello")));
  ASSERT_EQ(res.get_value(string("one")).to_array().get_value(string("hello")).as_string(), string("world"));
  ASSERT_TRUE(res.get_value(string("one")).to_array().has_key(string("world")));
  ASSERT_EQ(res.get_value(string("one")).to_array().get_value(string("world")).as_string(), string("hello"));

  ASSERT_TRUE(res.has_key(string("two")));
  ASSERT_TRUE(res.get_value(string("two")).is_array());
  ASSERT_TRUE(res.get_value(string("two")).to_array().has_key(string("a")));
  ASSERT_EQ(res.get_value(string("two")).to_array().get_value(string("a")).as_string(), string("a"));
  ASSERT_TRUE(res.get_value(string("two")).to_array().has_key(string("b")));
  ASSERT_EQ(res.get_value(string("two")).to_array().get_value(string("b")).as_string(), string("b"));
}

TEST(parsing_functions_test, test_parse_ini_string_scanner_mode_typed) {
  array<mixed> res(array_size(0, 0, true));

  res = f$parse_ini_string(string("[types vars] int=1500 float=3.14242 str=\"hello world\" bool=true"), false, INI_SCANNER_TYPED);
  ASSERT_EQ(res.size().string_size, 4);

}
