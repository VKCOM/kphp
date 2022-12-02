#include "runtime/ini.h"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

TEST(parsing_functions_test, test_parse_ini_string_empty) {
  array<mixed> res = f$parse_ini_string(string(""));
  ASSERT_TRUE(res.empty());
}

TEST(parsing_functions_test, test_parse_ini_string_scanner_mode_normal) {
  array<mixed> res(array_size(0, 0, true));

  // Sections false
  res = f$parse_ini_string(string("[one] hello=world world=hello [two] a=a b=b"));
  ASSERT_EQ(4, res.size().string_size);

  ASSERT_TRUE(res.has_key(string("hello")));
  ASSERT_TRUE(res.has_key(string("world")));
  ASSERT_TRUE(res.has_key(string("a")));
  ASSERT_TRUE(res.has_key(string("b")));

  res = f$parse_ini_string(string("hello=world world=hello a=a b=b"));
  ASSERT_EQ(4, res.size().string_size);

  ASSERT_TRUE(res.has_key(string("hello")));
  ASSERT_TRUE(res.has_key(string("world")));
  ASSERT_TRUE(res.has_key(string("a")));
  ASSERT_TRUE(res.has_key(string("b")));

  // Sections true
  res = f$parse_ini_string(string("[one] [two]"), true);

  ASSERT_EQ(2, res.size().string_size);
  ASSERT_TRUE(res.has_key(string("one")));
  ASSERT_TRUE(res.has_key(string("two")));

  res = f$parse_ini_string(string("[one] hello=world world=hello [two] a=a b=b"), true);

  ASSERT_EQ(2, res.size().string_size);
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

TEST(parsing_functions_test, test_parse_ini_string_scanner_mode_raw) {
  array<mixed> res(array_size(0, 0, true));

  // Sections false
  res = f$parse_ini_string(string(R"([one] hello='world' world="hello" [two] a='a' b=Off)"), false, INI_SCANNER_RAW);

  ASSERT_EQ(4, res.size().string_size);
  ASSERT_TRUE(res.has_key(string("hello")));
  ASSERT_STREQ(string("'world'").c_str(), res.get_value(string("hello")).to_string().c_str());
  ASSERT_TRUE(res.has_key(string("world")));
  ASSERT_STREQ(string("hello").c_str(), res.get_value(string("world")).to_string().c_str());
  ASSERT_TRUE(res.has_key(string("a")));
  ASSERT_STREQ(string("'a'").c_str(), res.get_value(string("a")).to_string().c_str());
  ASSERT_TRUE(res.has_key(string("b")));
  ASSERT_STREQ(string("Off").c_str(), res.get_value(string("b")).to_string().c_str());

  // Sections true
  res = f$parse_ini_string(string(R"([one] hello='world' world="hello" [two] a='a' b=Off)"), true, INI_SCANNER_RAW);

  ASSERT_EQ(2, res.size().string_size);
  ASSERT_TRUE(res.has_key(string("one")));
  ASSERT_TRUE(res.get_value(string("one")).to_array().has_key(string("hello")));
  ASSERT_STREQ(string("'world'").c_str(), res.get_value(string("one")).to_array().get_value(string("hello")).to_string().c_str());
  ASSERT_TRUE(res.get_value(string("one")).to_array().has_key(string("world")));
  ASSERT_STREQ(string("hello").c_str(), res.get_value(string("one")).to_array().get_value(string("world")).to_string().c_str());

  ASSERT_TRUE(res.has_key(string("two")));
  ASSERT_TRUE(res.get_value(string("two")).to_array().has_key(string("a")));
  ASSERT_STREQ(string("'a'").c_str(), res.get_value(string("two")).to_array().get_value(string("a")).to_string().c_str());
  ASSERT_TRUE(res.get_value(string("two")).to_array().has_key(string("b")));
  ASSERT_STREQ(string("Off").c_str(), res.get_value(string("two")).to_array().get_value(string("b")).to_string().c_str());
}

TEST(parsing_functions_test, test_parse_ini_string_scanner_mode_typed) {
  array<mixed> res(array_size(0, 0, true));

  // Sections false
  res = f$parse_ini_string(string("[types vars] int=1500 float=3.14242 str=\"hello world\" bool=true"), false, INI_SCANNER_TYPED);

  ASSERT_EQ(4, res.size().string_size);

  ASSERT_TRUE(res.get_value(string("int")).is_int());
  ASSERT_EQ(1500, res.get_value(string("int")).to_int());

  ASSERT_TRUE(res.get_value(string("float")).is_float());
  ASSERT_EQ(3.14242, res.get_value(string("float")).to_float());

  ASSERT_TRUE(res.get_value(string("str")).is_string());
  ASSERT_EQ(string("hello world"), res.get_value(string("str")).to_string());

  ASSERT_TRUE(res.get_value(string("bool")).is_bool());
  ASSERT_EQ(true, res.get_value(string("bool")).to_bool());

  // Sections true
  res = f$parse_ini_string(string("[types vars] int=1500 float=3.14242 str=\"hello world\" bool1=true bool2=Off"), true, INI_SCANNER_TYPED);

  ASSERT_TRUE(res.has_key(string("types vars")));
  ASSERT_TRUE(res.get_value(string("types vars")).to_array().get_value(string("int")).is_int());
  ASSERT_EQ(1500, res.get_value(string("types vars")).to_array().get_value(string("int")).to_int());

  ASSERT_TRUE(res.get_value(string("types vars")).to_array().get_value(string("float")).is_float());
  ASSERT_EQ(3.14242, res.get_value(string("types vars")).to_array().get_value(string("float")).to_float());

  ASSERT_TRUE(res.get_value(string("types vars")).to_array().get_value(string("str")).is_string());
  ASSERT_EQ(string("hello world"), res.get_value(string("types vars")).to_array().get_value(string("str")).to_string());

  ASSERT_TRUE(res.get_value(string("types vars")).to_array().get_value(string("bool1")).is_bool());
  ASSERT_EQ(true, res.get_value(string("types vars")).to_array().get_value(string("bool1")).to_bool());

  ASSERT_TRUE(res.get_value(string("types vars")).to_array().get_value(string("bool2")).is_bool());
  ASSERT_EQ(false, res.get_value(string("types vars")).to_array().get_value(string("bool2")).to_bool());
}

TEST(parsing_functions_test, test_parse_ini_file) {
  std::ofstream of("test.ini");

  if (of.is_open()) {
    of << "[types vars] "<< std::endl;
    of << "int=1500" << std::endl;
    of << "float=3.14242" << std::endl;
    of << "str1=\"hello world\"" << std::endl;
    of << "str2=\'hello world\'" << std::endl;
    of << "bool1=true" << std::endl;
    of << "bool2=Off";
    of.close();
  }

  array<mixed> res(array_size(0, 0, true));

  res = f$parse_ini_file(string("test.ini"));

  ASSERT_EQ(6, res.size().string_size);

  res = f$parse_ini_file(string("test.ini"), true);

  ASSERT_EQ(1, res.size().string_size);

  res = f$parse_ini_file(string("test.ini"), false, INI_SCANNER_RAW);

  ASSERT_EQ(6, res.size().string_size);

  res = f$parse_ini_file(string("test.ini"), true, INI_SCANNER_RAW);

  ASSERT_EQ(1, res.size().string_size);

  res = f$parse_ini_file(string("test.ini"), false, INI_SCANNER_TYPED);

  ASSERT_EQ(6, res.size().string_size);
}

