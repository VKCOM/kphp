#include <gtest/gtest.h>

#include "runtime/files.h"
#include "runtime/yaml.h"

TEST(yaml_test, test_yaml_string) {
  mixed example = string("string");
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_string());
  ASSERT_TRUE(example.as_string() == result.as_string());
}

TEST(yaml_test, test_yaml_empty_string) {
  mixed example = string("");
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_string());
  ASSERT_TRUE(example.as_string() == result.as_string());
}

TEST(yaml_test, test_yaml_int) {
  mixed example = 13;
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_int());
  ASSERT_EQ(example.as_int(), result.as_int());
}

TEST(yaml_test, test_yaml_float) {
  mixed example = 3.1416;
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_float());
  ASSERT_DOUBLE_EQ(example.as_double(), result.as_double());
}

TEST(yaml_test, test_yaml_int_as_string) {
  mixed example = string("13");
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_string());
  ASSERT_EQ(example.as_string(), result.as_string());
}

TEST(yaml_test, test_yaml_float_as_string) {
  mixed example = string("3.1416");
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_string());
  ASSERT_EQ(example.as_string(), result.as_string());
}

TEST(yaml_test, test_yaml_vector) {
  mixed example;
  example.push_back(string("string"));
  example.push_back(13);
  example.push_back(3.1416);
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[0].is_string());
  ASSERT_TRUE(example[0].as_string() == result[0].as_string());
  ASSERT_TRUE(result[1].is_int());
  ASSERT_EQ(example[1].as_int(), result[1].as_int());
  ASSERT_TRUE(result[2].is_float());
  ASSERT_DOUBLE_EQ(example[2].as_double(), result[2].as_double());
}

TEST(yaml_test, test_yaml_vector_recursive) {
  mixed example, copy;
  example.push_back(string("string"));
  example.push_back(13);
  example.push_back(3.1416);
  for (auto it = example.begin(); it != example.end(); ++it) {
    copy.push_back(it.get_value());
  }
  example.push_back(copy);
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result[3].is_array());
  ASSERT_TRUE(result[3].as_array().is_pseudo_vector());
  ASSERT_TRUE(result[3][0].is_string());
  ASSERT_TRUE(example[3][0].as_string() == result[3][0].as_string());
  ASSERT_TRUE(result[3][1].is_int());
  ASSERT_EQ(example[3][1].as_int(), result[3][1].as_int());
  ASSERT_TRUE(result[3][2].is_float());
  ASSERT_DOUBLE_EQ(example[3][2].as_double(), result[3][2].as_double());
}

TEST(yaml_test, test_yaml_bool) {
  mixed example;
  example.push_back(true);
  example.push_back(false);
  example.push_back(string("true"));
  example.push_back(string("false"));
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[0].is_bool());
  ASSERT_TRUE(example[0].as_bool() == result[0].as_bool());
  ASSERT_TRUE(result[1].is_bool());
  ASSERT_TRUE(example[1].as_bool() == result[1].as_bool());
  ASSERT_TRUE(result[2].is_string());
  ASSERT_TRUE(example[2].as_string() == result[2].as_string());
  ASSERT_TRUE(result[3].is_string());
  ASSERT_TRUE(example[3].as_string() == result[3].as_string());
}

TEST(yaml_test, test_yaml_map) {
  mixed example;
  example[string("first")] = string("string");
  example[string("second")] = 13;
  example[string("third")] = 3.1416;
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_array());
  ASSERT_FALSE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[string("first")].is_string());
  ASSERT_TRUE(example[string("first")].as_string() == result[string("first")].as_string());
  ASSERT_TRUE(result[string("second")].is_int());
  ASSERT_EQ(example[string("second")].as_int(), result[string("second")].as_int());
  ASSERT_TRUE(result[string("third")].is_float());
  ASSERT_DOUBLE_EQ(example[string("third")].as_double(), result[string("third")].as_double());
}

TEST(yaml_test, test_yaml_map_recursive) {
  mixed example, copy;
  example[string("first")] = string("string");
  example[string("second")] = 13;
  example[string("third")] = 3.1416;
  for (auto it = example.begin(); it != example.end(); ++it) {
    copy[it.get_key()] = it.get_value();
  }
  example[string("self")] = copy;
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result[string("self")].is_array());
  ASSERT_FALSE(result[string("self")].as_array().is_pseudo_vector());
  ASSERT_TRUE(result[string("self")][string("first")].is_string());
  ASSERT_TRUE(example[string("self")][string("first")].as_string() == result[string("self")][string("first")].as_string());
  ASSERT_TRUE(result[string("self")][string("second")].is_int());
  ASSERT_EQ(example[string("self")][string("second")].as_int(), result[string("self")][string("second")].as_int());
  ASSERT_TRUE(result[string("self")][string("third")].is_float());
  ASSERT_DOUBLE_EQ(example[string("self")][string("third")].as_double(), result[string("self")][string("third")].as_double());
}

TEST(yaml_test, test_yaml_empty_array) {
  array<mixed> empty_array;
  mixed result = f$yaml_parse(f$yaml_emit(empty_array));
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().empty());
}

TEST(yaml_test, test_yaml_null) {
  mixed example;
  mixed result = f$yaml_parse(f$yaml_emit(example));
  ASSERT_TRUE(result.is_null());
}

TEST(yaml_test, test_yaml_string_file) {
  mixed example = string("string");
  string filename("test_yaml_string");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_string());
  ASSERT_TRUE(example.as_string() == result.as_string());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_empty_string_file) {
  mixed example = string("");
  string filename("test_yaml_empty_string");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_string());
  ASSERT_TRUE(example.as_string() == result.as_string());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_int_file) {
  mixed example = 13;
  string filename("test_yaml_int");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_int());
  ASSERT_EQ(example.as_int(), result.as_int());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_float_file) {
  mixed example = 3.1416;
  string filename("test_yaml_float");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_float());
  ASSERT_DOUBLE_EQ(example.as_double(), result.as_double());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_int_as_string_file) {
  mixed example = string("13");
  string filename("test_yaml_int_as_string");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_string());
  ASSERT_EQ(example.as_string(), result.as_string());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_float_as_string_file) {
  mixed example = string("3.1416");
  string filename("test_yaml_float_as_string");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_string());
  ASSERT_EQ(example.as_string(), result.as_string());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_vector_file) {
  mixed example;
  example.push_back(string("string"));
  example.push_back(13);
  example.push_back(3.1416);
  string filename("test_yaml_vector");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[0].is_string());
  ASSERT_TRUE(example[0].as_string() == result[0].as_string());
  ASSERT_TRUE(result[1].is_int());
  ASSERT_EQ(example[1].as_int(), result[1].as_int());
  ASSERT_TRUE(result[2].is_float());
  ASSERT_DOUBLE_EQ(example[2].as_double(), result[2].as_double());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_vector_recursive_file) {
  mixed example, copy;
  example.push_back(string("string"));
  example.push_back(13);
  example.push_back(3.1416);
  for (auto it = example.begin(); it != example.end(); ++it) {
    copy.push_back(it.get_value());
  }
  example.push_back(copy);
  string filename("test_yaml_vector_recursive");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result[3].is_array());
  ASSERT_TRUE(result[3].as_array().is_pseudo_vector());
  ASSERT_TRUE(result[3][0].is_string());
  ASSERT_TRUE(example[3][0].as_string() == result[3][0].as_string());
  ASSERT_TRUE(result[3][1].is_int());
  ASSERT_EQ(example[3][1].as_int(), result[3][1].as_int());
  ASSERT_TRUE(result[3][2].is_float());
  ASSERT_DOUBLE_EQ(example[3][2].as_double(), result[3][2].as_double());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_bool_file) {
  mixed example;
  example.push_back(true);
  example.push_back(false);
  example.push_back(string("true"));
  example.push_back(string("false"));
  string filename("test_yaml_bool");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[0].is_bool());
  ASSERT_TRUE(example[0].as_bool() == result[0].as_bool());
  ASSERT_TRUE(result[1].is_bool());
  ASSERT_TRUE(example[1].as_bool() == result[1].as_bool());
  ASSERT_TRUE(result[2].is_string());
  ASSERT_TRUE(example[2].as_string() == result[2].as_string());
  ASSERT_TRUE(result[3].is_string());
  ASSERT_TRUE(example[3].as_string() == result[3].as_string());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_map_file) {
  mixed example;
  example[string("first")] = string("string");
  example[string("second")] = 13;
  example[string("third")] = 3.1416;
  string filename("test_yaml_map");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_array());
  ASSERT_FALSE(result.as_array().is_pseudo_vector());
  ASSERT_TRUE(result[string("first")].is_string());
  ASSERT_TRUE(example[string("first")].as_string() == result[string("first")].as_string());
  ASSERT_TRUE(result[string("second")].is_int());
  ASSERT_EQ(example[string("second")].as_int(), result[string("second")].as_int());
  ASSERT_TRUE(result[string("third")].is_float());
  ASSERT_DOUBLE_EQ(example[string("third")].as_double(), result[string("third")].as_double());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_map_recursive_file) {
  mixed example, copy;
  example[string("first")] = string("string");
  example[string("second")] = 13;
  example[string("third")] = 3.1416;
  for (auto it = example.begin(); it != example.end(); ++it) {
    copy[it.get_key()] = it.get_value();
  }
  example[string("self")] = copy;
  string filename("test_yaml_map_recursive");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result[string("self")].is_array());
  ASSERT_FALSE(result[string("self")].as_array().is_pseudo_vector());
  ASSERT_TRUE(result[string("self")][string("first")].is_string());
  ASSERT_TRUE(example[string("self")][string("first")].as_string() == result[string("self")][string("first")].as_string());
  ASSERT_TRUE(result[string("self")][string("second")].is_int());
  ASSERT_EQ(example[string("self")][string("second")].as_int(), result[string("self")][string("second")].as_int());
  ASSERT_TRUE(result[string("self")][string("third")].is_float());
  ASSERT_DOUBLE_EQ(example[string("self")][string("third")].as_double(), result[string("self")][string("third")].as_double());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_empty_array_file) {
  array<mixed> empty_array;
  string filename("test_yaml_empty_array");
  ASSERT_TRUE(f$yaml_emit_file(filename, empty_array));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_array());
  ASSERT_TRUE(result.as_array().empty());
  ASSERT_TRUE(f$unlink(filename));
}

TEST(yaml_test, test_yaml_null_file) {
  mixed example;
  string filename("test_yaml_null");
  ASSERT_TRUE(f$yaml_emit_file(filename, example));
  mixed result = f$yaml_parse_file(filename);
  ASSERT_TRUE(result.is_null());
  ASSERT_TRUE(f$unlink(filename));
}
