#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include "runtime/parsing_functions.h"

TEST(parsing_functions_test, test_get_env_var) {
  string str("APP_NAME=Laravel");
  string env_var = get_env_var(str);

  ASSERT_STREQ("APP_NAME", env_var.c_str());
  ASSERT_EQ(strlen("APP_NAME"), env_var.size());
}

TEST(parsing_functions_test, test_get_env_val) {
  string str("APP_NAME=Laravel");
  string env_val = get_env_val(str);

  ASSERT_STREQ("Laravel", env_val.c_str());
  ASSERT_EQ(strlen("Laravel"), env_val.size());
}

TEST(parsing_functions_test, test_parse_env_string) {
  string env_string = string(R"(APP_NAME=Laravel APP_ENV=local #APP_KEY=base64:mtlb8hldh5hZ0GlLzbhInsV531MSylspRI4JsmwVal8= APP_DEBUG=true T1="my" T2='my')");

  array<string> res(array_size(0, 0, true));

  res = f$parse_env_string(env_string);
  ASSERT_EQ(res.size().string_size, 5);

  ASSERT_TRUE(res.has_key(string("APP_NAME")));
  ASSERT_STREQ(res.get_value(string("APP_NAME")).c_str(), string("Laravel").c_str());
  ASSERT_TRUE(res.has_key(string("APP_ENV")));
  ASSERT_STREQ(res.get_value(string("APP_ENV")).c_str(), string("local").c_str());
  ASSERT_TRUE(res.has_key(string("APP_DEBUG")));
  ASSERT_STREQ(res.get_value(string("APP_DEBUG")).c_str(), string("true").c_str());
  ASSERT_TRUE(res.has_key(string("T1")));
  ASSERT_STREQ(res.get_value(string("T1")).c_str(), string("my").c_str());
  ASSERT_TRUE(res.has_key(string("T2")));
  ASSERT_STREQ(res.get_value(string("T2")).c_str(), string("my").c_str());
}

TEST(parsing_functions_test, test_parse_env_file) {
  std::ofstream of(".env.example");

  if (of.is_open()) {
    of << "APP_NAME=Laravel "<< std::endl;
    of << "APP_ENV=local" << std::endl;
    of << "APP_DEBUG=true" << std::endl;
    of.close();
  }

  array<string> res(array_size(0, 0, true));

  res = f$parse_env_file(string("file not found"));
  ASSERT_EQ(res.size().string_size, 0);

  res = f$parse_env_file(string(".env.example"));
  ASSERT_TRUE(res.has_key(string("APP_NAME")));
  ASSERT_STREQ(res.get_value(string("APP_NAME")).c_str(), string("Laravel").c_str());
  ASSERT_TRUE(res.has_key(string("APP_ENV")));
  ASSERT_STREQ(res.get_value(string("APP_ENV")).c_str(), string("local").c_str());
  ASSERT_TRUE(res.has_key(string("APP_DEBUG")));
  ASSERT_STREQ(res.get_value(string("APP_DEBUG")).c_str(), string("true").c_str());
}
