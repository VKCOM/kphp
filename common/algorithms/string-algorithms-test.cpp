#include <gtest/gtest.h>

#include "common/algorithms/string-algorithms.h"

template<class StringContainer>
struct TestTrimHelper {
  static void without_spaces() {
    StringContainer s{"asdf"};
    EXPECT_EQ(vk::ltrim(s), s);
    EXPECT_EQ(vk::rtrim(s), s);
    EXPECT_EQ(vk::trim(s), s);
  }

  static void only_spaces() {
    StringContainer s{"   "};
    EXPECT_EQ(vk::ltrim(s), "");
    EXPECT_EQ(vk::rtrim(s), "");
    EXPECT_EQ(vk::trim(s), "");
  }

  static void left_spaces() {
    StringContainer s{"   asdf"};
    EXPECT_EQ(vk::ltrim(s), "asdf");
    EXPECT_EQ(vk::rtrim(s), s);
    EXPECT_EQ(vk::trim(s), "asdf");
  }

  static void right_spaces() {
    StringContainer s{"asdf    "};
    EXPECT_EQ(vk::ltrim(s), s);
    EXPECT_EQ(vk::rtrim(s), "asdf");
    EXPECT_EQ(vk::trim(s), "asdf");
  }

  static void empty_string() {
    StringContainer s{""};
    EXPECT_EQ(vk::ltrim(s), "");
    EXPECT_EQ(vk::rtrim(s), "");
    EXPECT_EQ(vk::trim(s), "");
  }
};

TEST(string_algorithms_trim, without_spaces) {
  TestTrimHelper<vk::string_view>::without_spaces();
  TestTrimHelper<std::string>::without_spaces();
}

TEST(string_algorithms_trim, only_spaces) {
  TestTrimHelper<vk::string_view>::only_spaces();
  TestTrimHelper<std::string>::only_spaces();
}

TEST(string_algorithms_trim, left_spaces) {
  TestTrimHelper<vk::string_view>::left_spaces();
  TestTrimHelper<std::string>::left_spaces();
}

TEST(string_algorithms_trim, right_spaces) {
  TestTrimHelper<vk::string_view>::right_spaces();
  TestTrimHelper<std::string>::right_spaces();
}

TEST(string_algorithms_trim, empty_string) {
  TestTrimHelper<vk::string_view>::empty_string();
  TestTrimHelper<std::string>::empty_string();
}
