#include <gtest/gtest.h>
#include <string>

#include "runtime/confdata-keys.h"

namespace {

struct ExpectedKey {
  ConfdataFirstKeyType dots;
  const char *first;
  mixed second;
};

#define ASSERT_KEY(key_maker, expected)                                                                                                                        \
  do {                                                                                                                                                         \
    ASSERT_EQ(key_maker.get_first_key_type(), expected.dots);                                                                                                  \
    ASSERT_STREQ(key_maker.get_first_key().c_str(), expected.first);                                                                                           \
    ASSERT_TRUE(equals(key_maker.get_second_key(), expected.second));                                                                                          \
    ASSERT_STREQ(key_maker.make_first_key_copy().c_str(), expected.first);                                                                                     \
    ASSERT_TRUE(equals(key_maker.make_second_key_copy(), expected.second));                                                                                    \
  } while (0)

} // namespace

TEST(confdata_key_maker_test, test_default) {
  ConfdataKeyMaker key_maker;
  ExpectedKey expected{ConfdataFirstKeyType::simple_key, "", mixed{}};
  ASSERT_KEY(key_maker, expected);
}

TEST(confdata_key_maker_test, test_first_key_zero_dots) {
  std::unordered_map<std::string, ExpectedKey> samples{{"", {ConfdataFirstKeyType::simple_key, "", mixed{}}},
                                                       {"x", {ConfdataFirstKeyType::simple_key, "x", mixed{}}},
                                                       {"1", {ConfdataFirstKeyType::simple_key, "1", mixed{}}},
                                                       {"hello world!", {ConfdataFirstKeyType::simple_key, "hello world!", mixed{}}}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_empty_second) {
  std::unordered_map<std::string, ExpectedKey> samples{{".", {ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{}}}},
                                                       {"x.", {ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{}}}},
                                                       {"1.", {ConfdataFirstKeyType::one_dot_wildcard, "1.", mixed{string{}}}},
                                                       {"hello world!.", {ConfdataFirstKeyType::one_dot_wildcard, "hello world!.", mixed{string{}}}}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_string_second) {
  std::unordered_map<std::string, ExpectedKey> samples{{".g", {ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{"g"}}}},
                                                       {"x.gg", {ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{"gg"}}}},
                                                       {"1.two", {ConfdataFirstKeyType::one_dot_wildcard, "1.", mixed{string{"two"}}}},
                                                       {"hello .world!", {ConfdataFirstKeyType::one_dot_wildcard, "hello .", mixed{string{"world!"}}}},
                                                       {"big.9223372036854775808",
                                                        {ConfdataFirstKeyType::one_dot_wildcard, "big.", mixed{string{"9223372036854775808"}}}},
                                                       {"small.-9223372036854775809",
                                                        {ConfdataFirstKeyType::one_dot_wildcard, "small.", mixed{string{"-9223372036854775809"}}}},
                                                       {"bad_num0.-0", {ConfdataFirstKeyType::one_dot_wildcard, "bad_num0.", mixed{string{"-0"}}}},
                                                       {"bad_num1.1xx", {ConfdataFirstKeyType::one_dot_wildcard, "bad_num1.", mixed{string{"1xx"}}}},
                                                       {"bad_num2.012", {ConfdataFirstKeyType::one_dot_wildcard, "bad_num2.", mixed{string{"012"}}}}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_one_dot_number_second) {
  std::unordered_map<std::string, ExpectedKey> samples{{".123", {ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{123}}},
                                                       {"x.-15", {ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{-15}}},
                                                       {"x.48", {ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{48}}},
                                                       {"0.0", {ConfdataFirstKeyType::one_dot_wildcard, "0.", mixed{0}}},
                                                       {"max.9223372036854775807",
                                                        {ConfdataFirstKeyType::one_dot_wildcard, "max.", mixed{9223372036854775807L}}},
                                                       {"min.-9223372036854775808",
                                                        {ConfdataFirstKeyType::one_dot_wildcard, "min.", mixed{std::numeric_limits<int64_t>::min()}}}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.dots);
    ASSERT_KEY(key_maker, s.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_empty_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>>
    samples{{"..", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", mixed{string{}}},
                                  ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{"."}}})},
            {".ab.", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, ".ab.", mixed{string{}}},
                                    ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{"ab."}}})},
            {"x..", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x..", mixed{string{}}},
                                   ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{"."}}})},
            {"x.y.", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.y.", mixed{string{}}},
                                    ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{"y."}}})},
            {"1..", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1..", mixed{string{}}},
                                   ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", mixed{string{"."}}})},
            {"1.2.", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1.2.", mixed{string{}}},
                                    ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", mixed{string{"2."}}})},
            {"hello world!..", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello world!..", mixed{string{}}},
                                              ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello world!.", mixed{string{"."}}})},
            {"hello.world!.", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello.world!.", mixed{string{}}},
                                             ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello.", mixed{string{"world!."}}})}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_string_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>>
    samples{{"..g", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", mixed{string{"g"}}},
                                   ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{".g"}}})},
            {"x.g.g", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.g.", mixed{string{"g"}}},
                                     ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{"g.g"}}})},
            {"1.2.two", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "1.2.", mixed{string{"two"}}},
                                       ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "1.", mixed{string{"2.two"}}})},
            {"hello. .world!", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "hello. .", mixed{string{"world!"}}},
                                              ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "hello.", mixed{string{" .world!"}}})},
            {"big.num.9223372036854775808",
             std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "big.num.", mixed{string{"9223372036854775808"}}},
                            ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "big.", mixed{string{"num.9223372036854775808"}}})},
            {"small.num.-9223372036854775809",
             std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "small.num.", mixed{string{"-9223372036854775809"}}},
                            ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "small.", mixed{string{"num.-9223372036854775809"}}})},
            {"bad_num.0.-0", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.0.", mixed{string{"-0"}}},
                                            ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", mixed{string{"0.-0"}}})},
            {"bad_num.1.1xx", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.1.", mixed{string{"1xx"}}},
                                             ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", mixed{string{"1.1xx"}}})},
            {"bad_num.2.012", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "bad_num.2.", mixed{string{"012"}}},
                                             ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "bad_num.", mixed{string{"2.012"}}})}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}

TEST(confdata_key_maker_test, test_first_key_two_dots_number_second) {
  std::unordered_map<std::string, std::pair<ExpectedKey, ExpectedKey>>
    samples{{"..123", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "..", mixed{123}},
                                     ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, ".", mixed{string{".123"}}})},
            {"x.y.-15", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x.y.", mixed{-15}},
                                       ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{"y.-15"}}})},
            {"x..48", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "x..", mixed{48}},
                                     ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "x.", mixed{string{".48"}}})},
            {"0.-1.0", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "0.-1.", mixed{0}},
                                      ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "0.", mixed{string{"-1.0"}}})},
            {"max.n.2147483647", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "max.n.", mixed{2147483647}},
                                                ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "max.", mixed{string{"n.2147483647"}}})},
            {"min.n.-2147483648", std::make_pair(ExpectedKey{ConfdataFirstKeyType::two_dots_wildcard, "min.n.", mixed{int{-2147483648}}},
                                                 ExpectedKey{ConfdataFirstKeyType::one_dot_wildcard, "min.", mixed{string{"n.-2147483648"}}})}};

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    ASSERT_EQ(key_maker.update(s.first.c_str(), s.first.size()), s.second.first.dots);
    ASSERT_KEY(key_maker, s.second.first);
    key_maker.forcibly_change_first_key_wildcard_dots_from_two_to_one();
    ASSERT_KEY(key_maker, s.second.second);
  }
}

TEST(confdata_key_maker_test, test_update_with_predefined_wildcard) {
  struct Sample {
    std::string key;
    int32_t wildcard_len;
    ExpectedKey expected;
  };
  std::vector<Sample> samples{
    {"abcd", 3, {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{"d"}}}},
    {"123abc", 3, {ConfdataFirstKeyType::predefined_wildcard, "123", mixed{string{"abc"}}}},
    {"abc123", 3, {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{123}}},
    {"abc123", 1, {ConfdataFirstKeyType::predefined_wildcard, "a", mixed{string{"bc123"}}}},
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    key_maker.update_with_predefined_wildcard(s.key.c_str(), s.key.size(), s.wildcard_len);
    ASSERT_KEY(key_maker, s.expected);
  }
}

TEST(confdata_key_maker_test, test_update_with_predefined_wildcard_auto) {
  ConfdataPredefinedWildcards predefined_wildcards;
  predefined_wildcards.set_wildcards({"abc", "abc.xyz", "cde", "cd", "hello."});

  std::unordered_map<std::string, ExpectedKey> samples{
    {"abc", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{}}}},
    {"abc.", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{"."}}}},
    {"abcd", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{"d"}}}},
    {"abc123", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{123}}},
    {"abc.xyz", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{".xyz"}}}},
    {"abc.xyz.uvz", {ConfdataFirstKeyType::predefined_wildcard, "abc", mixed{string{".xyz.uvz"}}}},
    {"cd", {ConfdataFirstKeyType::predefined_wildcard, "cd", mixed{string{}}}},
    {"cdxxx", {ConfdataFirstKeyType::predefined_wildcard, "cd", mixed{string{"xxx"}}}},
    {"cde.xyz", {ConfdataFirstKeyType::predefined_wildcard, "cd", mixed{string{"e.xyz"}}}},

    {"a", {ConfdataFirstKeyType::simple_key, "a", mixed{}}},
    {"ab", {ConfdataFirstKeyType::simple_key, "ab", mixed{}}},
    {"hello world", {ConfdataFirstKeyType::simple_key, "hello world", mixed{}}},
    {"foo", {ConfdataFirstKeyType::simple_key, "foo", mixed{}}},

    {"foo.", {ConfdataFirstKeyType::one_dot_wildcard, "foo.", mixed{string{}}}},
    {"foo.bar", {ConfdataFirstKeyType::one_dot_wildcard, "foo.", mixed{string{"bar"}}}},
    {"hello.world", {ConfdataFirstKeyType::one_dot_wildcard, "hello.", mixed{string{"world"}}}},

    {"foo.bar.", {ConfdataFirstKeyType::two_dots_wildcard, "foo.bar.", mixed{string{}}}},
    {"foo.bar.baz", {ConfdataFirstKeyType::two_dots_wildcard, "foo.bar.", mixed{string{"baz"}}}},
    {"hello.wo.ld", {ConfdataFirstKeyType::two_dots_wildcard, "hello.wo.", mixed{string{"ld"}}}},
  };

  ConfdataKeyMaker key_maker;
  for (const auto &s : samples) {
    key_maker.update(s.first.c_str(), s.first.size(), predefined_wildcards);
    ASSERT_KEY(key_maker, s.second);
  }
}
