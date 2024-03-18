#include <gtest/gtest.h>

#include "runtime/json-writer.h"

using namespace impl_;

TEST(json_writer, not_filled) {
  JsonWriter writer;
  ASSERT_FALSE(writer.is_complete());
}

TEST(json_writer, boolean) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_bool(true));
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "true");
}

TEST(json_writer, integer) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_int(42));
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "42");
}

TEST(json_writer, floating) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_double(1.234));
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "1.234");
}

TEST(json_writer, string) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_string(string{"foo"}));
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "\"foo\"");
}

TEST(json_writer, null) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_null());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "null");
}

TEST(json_writer, empty_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "{}");
}

TEST(json_writer, object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());

  ASSERT_TRUE(writer.write_key("b"));
  ASSERT_TRUE(writer.write_bool(true));

  ASSERT_TRUE(writer.write_key("i"));
  ASSERT_TRUE(writer.write_int(9));

  ASSERT_TRUE(writer.write_key("d"));
  ASSERT_TRUE(writer.write_double(1.2));

  ASSERT_TRUE(writer.write_key("s"));
  ASSERT_TRUE(writer.write_string(string{"s"}));

  ASSERT_TRUE(writer.write_key("n"));
  ASSERT_TRUE(writer.write_null());

  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"({"b":true,"i":9,"d":1.2,"s":"s","n":null})");
}

TEST(json_writer, empty_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.end_array());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), "[]");
}

TEST(json_writer, array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());

  ASSERT_TRUE(writer.write_bool(true));
  ASSERT_TRUE(writer.write_int(9));
  ASSERT_TRUE(writer.write_double(1.2));
  ASSERT_TRUE(writer.write_string(string{"s"}));
  ASSERT_TRUE(writer.write_null());

  ASSERT_TRUE(writer.end_array());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"([true,9,1.2,"s",null])");
}

TEST(json_writer, object_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());

  ASSERT_TRUE(writer.write_key("b"));
  ASSERT_TRUE(writer.write_bool(true));

  ASSERT_TRUE(writer.write_key("a1"));
  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.write_int(9));
  ASSERT_TRUE(writer.write_null());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.write_key("a2"));
  ASSERT_TRUE(writer.start_array());

  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());

  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.write_key("d"));
  ASSERT_TRUE(writer.write_double(1.2));
  ASSERT_TRUE(writer.end_object());

  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.write_int(9));
  ASSERT_TRUE(writer.write_null());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"({"b":true,"a1":[9,null],"a2":[{},{"d":1.2},[],[9,null]]})");
}

TEST(json_writer, array_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());

  ASSERT_TRUE(writer.write_bool(false));

  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());

  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.write_key("d"));
  ASSERT_TRUE(writer.write_double(1.2));
  ASSERT_TRUE(writer.end_object());

  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.write_int(9));
  ASSERT_TRUE(writer.write_null());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.end_array());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"([false,{},{"d":1.2},[],[9,null]])");
}

TEST(json_writer, floating_precision) {
  const double d = 1.1234567;
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());

  ASSERT_TRUE(writer.write_key("default"));
  ASSERT_TRUE(writer.write_double(d));

  for (std::size_t i = 1; i < 10; ++i) {
    writer.set_double_precision(i);
    ASSERT_TRUE(writer.write_key(std::to_string(i)));
    ASSERT_TRUE(writer.write_double(d));
  }

  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(),
               R"({"default":1.1234567,"1":1.1,"2":1.12,"3":1.123,"4":1.1235,"5":1.12346,"6":1.123457,"7":1.1234567,"8":1.1234567,"9":1.1234567})");
}

TEST(json_writer, floating_preserve_zero_fraction) {
  JsonWriter writer{false, true};
  ASSERT_TRUE(writer.start_object());

  ASSERT_TRUE(writer.write_key("a"));
  ASSERT_TRUE(writer.write_double(5));

  ASSERT_TRUE(writer.write_key("b"));
  ASSERT_TRUE(writer.write_double(7.0));

  writer.set_double_precision(1);

  ASSERT_TRUE(writer.write_key("c"));
  ASSERT_TRUE(writer.write_double(5));

  ASSERT_TRUE(writer.write_key("d"));
  ASSERT_TRUE(writer.write_double(7.0));

  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"({"a":5.0,"b":7.0,"c":5.0,"d":7.0})");
}

TEST(json_writer, pretty_print) {
  JsonWriter writer{true};
  ASSERT_TRUE(writer.start_object());

  ASSERT_TRUE(writer.write_key("a"));
  ASSERT_TRUE(writer.write_double(5));

  ASSERT_TRUE(writer.write_key("b"));
  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());

  ASSERT_TRUE(writer.write_key("c"));
  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.write_key("d"));
  ASSERT_TRUE(writer.start_array());
  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.write_bool(false));
  ASSERT_TRUE(writer.write_string(string{"foo"}));
  ASSERT_TRUE(writer.end_array());

  ASSERT_TRUE(writer.end_object());
  ASSERT_TRUE(writer.is_complete());
  ASSERT_STREQ(writer.get_final_json().c_str(), R"({
    "a": 5,
    "b": {},
    "c": [],
    "d": [
        {},
        false,
        "foo"
    ]
})");
}

TEST(json_writer_error, key_at_root) {
  JsonWriter writer;
  ASSERT_FALSE(writer.write_key("k"));
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "json key is allowed only inside object");
}

TEST(json_writer_error, key_in_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());
  ASSERT_FALSE(writer.write_key("k"));
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "json key is allowed only inside object");
}

TEST(json_writer_error, twice_values_in_root) {
  JsonWriter writer;
  ASSERT_TRUE(writer.write_bool(true));
  ASSERT_FALSE(writer.write_int(17));
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "attempt to set value twice in a root of json");
}

TEST(json_writer_error, braces_square_brackets) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());
  ASSERT_FALSE(writer.end_array());
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "attempt to enclosure { with ]");
}

TEST(json_writer_error, square_brackets_braces) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());
  ASSERT_FALSE(writer.end_object());
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "attempt to enclosure [ with }");
}

TEST(json_writer_error, incomplete_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_array());
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "");
}

TEST(json_writer_error, incomplete_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "");
}

TEST(json_writer_error, brace_disbalance) {
  JsonWriter writer;
  ASSERT_TRUE(writer.start_object());
  ASSERT_TRUE(writer.end_object());
  ASSERT_FALSE(writer.end_object());
  ASSERT_FALSE(writer.is_complete());
  ASSERT_STREQ(writer.get_error().c_str(), "brace disbalance");
}

TEST(json_writer_error, stack_overflow) {
  JsonWriter writer;
  for (std::size_t i = 0; i < 128; ++i) {
    ASSERT_TRUE(writer.start_array());
  }

  // there is no stack overflow error anymore
  ASSERT_FALSE(writer.is_complete());
  ASSERT_TRUE(writer.get_error().empty());

  for (std::size_t i = 0; i < 128; ++i) {
    ASSERT_TRUE(writer.end_array());
  }
  ASSERT_TRUE(writer.is_complete());
  ASSERT_TRUE(writer.get_error().empty());
}
