#include <gtest/gtest.h>
#include <string>
#include <unordered_map>

#include "runtime/confdata-global-manager.h"

namespace {

struct ExpectedKey {
  ConfdataFirstKeyType dots;
  const char *first;
  var second;
};

#define ASSERT_KEY(key_maker, expected)                                   \
do {                                                                      \
  ASSERT_EQ(key_maker.get_first_key_type(), expected.dots);               \
  ASSERT_STREQ(key_maker.get_first_key().c_str(), expected.first);        \
  ASSERT_TRUE(equals(key_maker.get_second_key(), expected.second));       \
  ASSERT_STREQ(key_maker.make_first_key_copy().c_str(), expected.first);  \
  ASSERT_TRUE(equals(key_maker.make_second_key_copy(), expected.second)); \
} while(0)

} // namespace


TEST(confdata_key_maker_test, test_defualt) {
  ConfdataKeyMaker key_maker;
  ExpectedKey expeced{ConfdataFirstKeyType::simple_key, "", var{}};
  ASSERT_KEY(key_maker, expeced);
}

TEST(confdata_key_maker_test, test_first_key_zero_dots) {
  std::unordered_map<std::string, ExpectedKey> samples{
    {"",             {ConfdataFirstKeyType::simple_key, "",             var{}}},
    {"x",            {ConfdataFirstKeyType::simple_key, "x",            var{}}},
    {"1",            {ConfdataFirstKeyType::simple_key, "1",            var{}}},
    {"hello world!", {ConfdataFirstKeyType::simple_key, "hello world!", var{}}}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_empty_second) {
  std::unordered_map<std::string, ExpectedKey> samples{
    {".",             {ConfdataFirstKeyType::one_dot_wildcard, ".",             var{string{}}}},
    {"x.",            {ConfdataFirstKeyType::one_dot_wildcard, "x.",            var{string{}}}},
    {"1.",            {ConfdataFirstKeyType::one_dot_wildcard, "1.",            var{string{}}}},
    {"hello world!.", {ConfdataFirstKeyType::one_dot_wildcard, "hello world!.", var{string{}}}}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_string_second) {
  std::unordered_map<std::string, ExpectedKey> samples{
    {".g",                {ConfdataFirstKeyType::one_dot_wildcard, ".",         var{string{"g"}}}},
    {"x.gg",              {ConfdataFirstKeyType::one_dot_wildcard, "x.",        var{string{"gg"}}}},
    {"1.two",             {ConfdataFirstKeyType::one_dot_wildcard, "1.",        var{string{"two"}}}},
    {"hello .world!",     {ConfdataFirstKeyType::one_dot_wildcard, "hello .",   var{string{"world!"}}}},
    {"big.9223372036854775808",    {ConfdataFirstKeyType::one_dot_wildcard, "big.",      var{string{"9223372036854775808"}}}},
    {"small.-9223372036854775809", {ConfdataFirstKeyType::one_dot_wildcard, "small.",    var{string{"-9223372036854775809"}}}},
    {"bad_num0.-0",       {ConfdataFirstKeyType::one_dot_wildcard, "bad_num0.", var{string{"-0"}}}},
    {"bad_num1.1xx",      {ConfdataFirstKeyType::one_dot_wildcard, "bad_num1.", var{string{"1xx"}}}},
    {"bad_num2.012",      {ConfdataFirstKeyType::one_dot_wildcard, "bad_num2.", var{string{"012"}}}}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_number_second) {
  std::unordered_map<std::string, ExpectedKey> samples{
    {".123",            {ConfdataFirstKeyType::one_dot_wildcard, ".",    var{123}}},
    {"x.-15",           {ConfdataFirstKeyType::one_dot_wildcard, "x.",   var{-15}}},
    {"x.48",            {ConfdataFirstKeyType::one_dot_wildcard, "x.",   var{48}}},
    {"0.0",             {ConfdataFirstKeyType::one_dot_wildcard, "0.",   var{0}}},
    {"max.9223372036854775807",  {ConfdataFirstKeyType::one_dot_wildcard, "max.", var{9223372036854775807L}}},
    {"min.-9223372036854775808", {ConfdataFirstKeyType::one_dot_wildcard, "min.", var{std::numeric_limits<int64_t>::min()}}}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_empty_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>> samples{
    {"..",             std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", var{string{"."}}}
    )},
    {".ab.",           std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, ".ab.", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", var{string{"ab."}}}
    )},
    {"x..",            std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x..", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", var{string{"."}}}
    )},
    {"x.y.",           std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.y.", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", var{string{"y."}}}
    )},
    {"1..",            std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1..", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", var{string{"."}}}
    )},
    {"1.2.",           std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1.2.", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", var{string{"2."}}}
    )},
    {"hello world!..", std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello world!..", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello world!.", var{string{"."}}}
    )},
    {"hello.world!.",  std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello.world!.", var{string{}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello.", var{string{"world!."}}}
    )}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_string_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>> samples{
    {"..g",                   std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", var{string{"g"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", var{string{".g"}}}
    )},
    {"x.g.g",                 std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.g.", var{string{"g"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", var{string{"g.g"}}}
    )},
    {"1.2.two",               std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1.2.", var{string{"two"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", var{string{"2.two"}}}
    )},
    {"hello. .world!",        std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello. .", var{string{"world!"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello.", var{string{" .world!"}}}
    )},
    {"big.num.9223372036854775808",    std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "big.num.", var{string{"9223372036854775808"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "big.", var{string{"num.9223372036854775808"}}}
    )},
    {"small.num.-9223372036854775809", std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "small.num.", var{string{"-9223372036854775809"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "small.", var{string{"num.-9223372036854775809"}}}
    )},
    {"bad_num.0.-0",          std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.0.", var{string{"-0"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", var{string{"0.-0"}}}
    )},
    {"bad_num.1.1xx",         std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.1.", var{string{"1xx"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", var{string{"1.1xx"}}}
    )},
    {"bad_num.2.012",         std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.2.", var{string{"012"}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", var{string{"2.012"}}}
    )}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_number_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>> samples{
    {"..123",             std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", var{123}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", var{string{".123"}}}
    )},
    {"x.y.-15",           std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.y.", var{-15}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", var{string{"y.-15"}}}
    )},
    {"x..48",             std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x..", var{48}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", var{string{".48"}}}
    )},
    {"0.-1.0",            std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "0.-1.", var{0}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "0.", var{string{"-1.0"}}}
    )},
    {"max.n.2147483647",  std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "max.n.", var{2147483647}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "max.", var{string{"n.2147483647"}}}
    )},
    {"min.n.-2147483648", std::make_pair(
      ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "min.n.", var{int{-2147483648}}},
      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "min.", var{string{"n.-2147483648"}}}
    )}
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}
