#include <gtest/gtest.h>

#include "compiler/utils/string-utils.h"

TEST(string_utils, snake_case) {
  ASSERT_EQ(transform_to_snake_case(""), "");
  ASSERT_EQ(transform_to_snake_case("blahblah"), "blahblah");
  ASSERT_EQ(transform_to_snake_case("blah_blah"), "blah_blah");

  ASSERT_EQ(transform_to_snake_case("blahBlah"), "blah_blah");
  ASSERT_EQ(transform_to_snake_case("blah_Blah"), "blah_blah");
  ASSERT_EQ(transform_to_snake_case("Blahblah"), "blahblah");
  ASSERT_EQ(transform_to_snake_case("BlahBlahBlah"), "blah_blah_blah");
  ASSERT_EQ(transform_to_snake_case("_blah"), "_blah");
  ASSERT_EQ(transform_to_snake_case("_blahBlah"), "_blah_blah");
  ASSERT_EQ(transform_to_snake_case("blah_"), "blah_");
  ASSERT_EQ(transform_to_snake_case("blahBlah_"), "blah_blah_");
}

TEST(string_utils, camel_case) {
  ASSERT_EQ(transform_to_camel_case(""), "");
  ASSERT_EQ(transform_to_camel_case("a"), "a");
  ASSERT_EQ(transform_to_camel_case("ab"), "ab");
  ASSERT_EQ(transform_to_camel_case("_"), "_");
  ASSERT_EQ(transform_to_camel_case("_a"), "_a");
  ASSERT_EQ(transform_to_camel_case("blahblah"), "blahblah");
  ASSERT_EQ(transform_to_camel_case("blahBlah"), "blahBlah");
  ASSERT_EQ(transform_to_camel_case("BlahBlaH"), "BlahBlaH");

  ASSERT_EQ(transform_to_camel_case("blah_blah"), "blahBlah");
  ASSERT_EQ(transform_to_camel_case("blah_Blah"), "blahBlah");
  ASSERT_EQ(transform_to_camel_case("blah_blah_blah"), "blahBlahBlah");
  ASSERT_EQ(transform_to_camel_case("_blah"), "_blah");
  ASSERT_EQ(transform_to_camel_case("_blah_blah"), "_blahBlah");
  ASSERT_EQ(transform_to_camel_case("blah_"), "blah_");
  ASSERT_EQ(transform_to_camel_case("blah_blah_"), "blahBlah_");
}
