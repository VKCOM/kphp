#include <gtest/gtest.h>

#include "runtime/json-writer.h"

using namespace impl_;

TEST(json_writer, not_filled) {
  JsonWriter writer;
  ASSERT_FALSE(writer.IsComplete());
}

TEST(json_writer, boolean) {
  JsonWriter writer;
  ASSERT_TRUE(writer.Bool(true));
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "true");
}

TEST(json_writer, integer) {
  JsonWriter writer;
  ASSERT_TRUE(writer.Int64(42));
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "42");
}

TEST(json_writer, floating) {
  JsonWriter writer;
  ASSERT_TRUE(writer.Double(1.234));
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "1.234");
}

TEST(json_writer, string) {
  JsonWriter writer;
  ASSERT_TRUE(writer.String(string{"foo"}));
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "\"foo\"");
}

TEST(json_writer, null) {
  JsonWriter writer;
  ASSERT_TRUE(writer.Null());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "null");
}

TEST(json_writer, empty_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "{}");
}

TEST(json_writer, object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());

  ASSERT_TRUE(writer.Key("b"));
  ASSERT_TRUE(writer.Bool(true));

  ASSERT_TRUE(writer.Key("i"));
  ASSERT_TRUE(writer.Int64(9));

  ASSERT_TRUE(writer.Key("d"));
  ASSERT_TRUE(writer.Double(1.2));

  ASSERT_TRUE(writer.Key("s"));
  ASSERT_TRUE(writer.String(string{"s"}));

  ASSERT_TRUE(writer.Key("n"));
  ASSERT_TRUE(writer.Null());

  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"({"b":true,"i":9,"d":1.2,"s":"s","n":null})");
}

TEST(json_writer, empty_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.EndArray());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), "[]");
}

TEST(json_writer, array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());

  ASSERT_TRUE(writer.Bool(true));
  ASSERT_TRUE(writer.Int64(9));
  ASSERT_TRUE(writer.Double(1.2));
  ASSERT_TRUE(writer.String(string{"s"}));
  ASSERT_TRUE(writer.Null());

  ASSERT_TRUE(writer.EndArray());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"([true,9,1.2,"s",null])");
}

TEST(json_writer, object_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());

  ASSERT_TRUE(writer.Key("b"));
  ASSERT_TRUE(writer.Bool(true));

  ASSERT_TRUE(writer.Key("a1"));
  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.Int64(9));
  ASSERT_TRUE(writer.Null());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.Key("a2"));
  ASSERT_TRUE(writer.StartArray());

  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());

  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.Key("d"));
  ASSERT_TRUE(writer.Double(1.2));
  ASSERT_TRUE(writer.EndObject());

  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.Int64(9));
  ASSERT_TRUE(writer.Null());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"({"b":true,"a1":[9,null],"a2":[{},{"d":1.2},[],[9,null]]})");
}

TEST(json_writer, array_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());

  ASSERT_TRUE(writer.Bool(false));

  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());

  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.Key("d"));
  ASSERT_TRUE(writer.Double(1.2));
  ASSERT_TRUE(writer.EndObject());

  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.Int64(9));
  ASSERT_TRUE(writer.Null());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.EndArray());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"([false,{},{"d":1.2},[],[9,null]])");
}

TEST(json_writer, floating_precision) {
  const double d = 1.1234567;
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());

  ASSERT_TRUE(writer.Key("default"));
  ASSERT_TRUE(writer.Double(d));

  for (std::size_t i = 1; i < 10; ++i) {
    writer.SetDoublePrecision(i);
    ASSERT_TRUE(writer.Key(std::to_string(i)));
    ASSERT_TRUE(writer.Double(d));
  }

  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"({"default":1.1234567,"1":1.1,"2":1.12,"3":1.123,"4":1.1235,"5":1.12346,"6":1.123457,"7":1.1234567,"8":1.1234567,"9":1.1234567})");
}

TEST(json_writer, floating_preserve_zero_fraction) {
  JsonWriter writer{false, true};
  ASSERT_TRUE(writer.StartObject());

  ASSERT_TRUE(writer.Key("a"));
  ASSERT_TRUE(writer.Double(5));

  ASSERT_TRUE(writer.Key("b"));
  ASSERT_TRUE(writer.Double(7.0));

  writer.SetDoublePrecision(1);

  ASSERT_TRUE(writer.Key("c"));
  ASSERT_TRUE(writer.Double(5));

  ASSERT_TRUE(writer.Key("d"));
  ASSERT_TRUE(writer.Double(7.0));

  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"({"a":5.0,"b":7.0,"c":5.0,"d":7.0})");
}

TEST(json_writer, pretty_print) {
  JsonWriter writer{true};
  ASSERT_TRUE(writer.StartObject());

  ASSERT_TRUE(writer.Key("a"));
  ASSERT_TRUE(writer.Double(5));

  ASSERT_TRUE(writer.Key("b"));
  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());

  ASSERT_TRUE(writer.Key("c"));
  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.Key("d"));
  ASSERT_TRUE(writer.StartArray());
  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.Bool(false));
  ASSERT_TRUE(writer.String(string{"foo"}));
  ASSERT_TRUE(writer.EndArray());

  ASSERT_TRUE(writer.EndObject());
  ASSERT_TRUE(writer.IsComplete());
  ASSERT_STREQ(writer.GetJson().c_str(), R"({
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
  ASSERT_FALSE(writer.Key("k"));
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "json key is allowed only inside object");
}

TEST(json_writer_error, key_in_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());
  ASSERT_FALSE(writer.Key("k"));
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "json key is allowed only inside object");
}

TEST(json_writer_error, twice_values_in_root) {
  JsonWriter writer;
  ASSERT_TRUE(writer.Bool(true));
  ASSERT_FALSE(writer.Int64(17));
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "attempt to set value twice in a root of json");
}

TEST(json_writer_error, braces_square_brackets) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());
  ASSERT_FALSE(writer.EndArray());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "attempt to enclosure { with ]");
}

TEST(json_writer_error, square_brackets_braces) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());
  ASSERT_FALSE(writer.EndObject());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "attempt to enclosure [ with }");
}

TEST(json_writer_error, incomplete_array) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartArray());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "");
}

TEST(json_writer_error, incomplete_object) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "");
}

TEST(json_writer_error, brace_disbalance) {
  JsonWriter writer;
  ASSERT_TRUE(writer.StartObject());
  ASSERT_TRUE(writer.EndObject());
  ASSERT_FALSE(writer.EndObject());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "brace disbalance");
}

TEST(json_writer_error, stack_overflow) {
  JsonWriter writer;
  for (std::size_t i = 0; i < 64; ++i) {
    ASSERT_TRUE(writer.StartArray());
  }
  ASSERT_FALSE(writer.StartArray());
  ASSERT_FALSE(writer.IsComplete());
  ASSERT_STREQ(writer.GetError().c_str(), "stack overflow, max depth level is 64");
}
