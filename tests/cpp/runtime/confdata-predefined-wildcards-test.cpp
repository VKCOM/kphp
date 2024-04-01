#include <gtest/gtest.h>
#include <set>

#include "runtime/confdata-keys.h"

TEST(confdata_predefined_wildcards_test, test_empty) {
  ConfdataPredefinedWildcards w;

  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key(".").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("..").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc.").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc.def").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc.def.").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc.def.ghi").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("abc.def.ghi.").empty());

  ASSERT_EQ(0, w.get_max_wildcards_for_element());

  ASSERT_FALSE(w.has_wildcard(""));
  ASSERT_FALSE(w.has_wildcard("."));
  ASSERT_FALSE(w.has_wildcard("abc"));
  ASSERT_FALSE(w.has_wildcard("abc."));
  ASSERT_FALSE(w.has_wildcard("abc.def"));
  ASSERT_FALSE(w.has_wildcard("abc.def."));

  ASSERT_EQ(w.detect_first_key_type(""), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc"), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def"), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi"), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi."), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi.jkl"), ConfdataFirstKeyType::simple_key);

  ASSERT_EQ(w.detect_first_key_type("."), ConfdataFirstKeyType::one_dot_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc."), ConfdataFirstKeyType::one_dot_wildcard);

  ASSERT_EQ(w.detect_first_key_type(".."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc.."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type(".abc."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc.def."), ConfdataFirstKeyType::two_dots_wildcard);

  ASSERT_FALSE(w.has_wildcard_for_key(""));
  ASSERT_FALSE(w.has_wildcard_for_key("."));
  ASSERT_FALSE(w.has_wildcard_for_key("abc"));
  ASSERT_FALSE(w.has_wildcard_for_key("abc."));
  ASSERT_FALSE(w.has_wildcard_for_key("abc.def"));
  ASSERT_FALSE(w.has_wildcard_for_key("abc.def."));
}

TEST(confdata_predefined_wildcards_test, test_simple) {
  ConfdataPredefinedWildcards w;

  w.set_wildcards({"a", "ab", "abc", "c"});
  auto make_wildcards_len_range = [&w](vk::string_view key) {
    const auto range = w.make_predefined_wildcard_len_range_by_key(key);
    return std::set<size_t>{range.begin(), range.end()};
  };

  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key(".").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("..").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("xyz").empty());
  ASSERT_TRUE(w.make_predefined_wildcard_len_range_by_key("xyz.abc").empty());

  ASSERT_EQ(make_wildcards_len_range("a"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("acb"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("a.bc"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("axyz"), std::set<size_t>({1}));

  ASSERT_EQ(make_wildcards_len_range("ab"), std::set<size_t>({1, 2}));
  ASSERT_EQ(make_wildcards_len_range("abb"), std::set<size_t>({1, 2}));
  ASSERT_EQ(make_wildcards_len_range("ab.c"), std::set<size_t>({1, 2}));
  ASSERT_EQ(make_wildcards_len_range("abxyz"), std::set<size_t>({1, 2}));

  ASSERT_EQ(make_wildcards_len_range("abc"), std::set<size_t>({1, 2, 3}));
  ASSERT_EQ(make_wildcards_len_range("abc.def"), std::set<size_t>({1, 2, 3}));
  ASSERT_EQ(make_wildcards_len_range("abcdef"), std::set<size_t>({1, 2, 3}));

  ASSERT_EQ(make_wildcards_len_range("c"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("cab"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("cccc"), std::set<size_t>({1}));
  ASSERT_EQ(make_wildcards_len_range("c.axyz"), std::set<size_t>({1}));

  ASSERT_EQ(3, w.get_max_wildcards_for_element());

  ASSERT_FALSE(w.has_wildcard(""));
  ASSERT_FALSE(w.has_wildcard("."));
  ASSERT_FALSE(w.has_wildcard("a."));
  ASSERT_FALSE(w.has_wildcard("abc."));
  ASSERT_FALSE(w.has_wildcard("abc.def"));

  ASSERT_TRUE(w.has_wildcard("a"));
  ASSERT_TRUE(w.has_wildcard("ab"));
  ASSERT_TRUE(w.has_wildcard("abc"));
  ASSERT_TRUE(w.has_wildcard("c"));

  ASSERT_EQ(w.detect_first_key_type(""), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def"), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi"), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi."), ConfdataFirstKeyType::simple_key);
  ASSERT_EQ(w.detect_first_key_type("abc.def.ghi.jkl"), ConfdataFirstKeyType::simple_key);

  ASSERT_EQ(w.detect_first_key_type("."), ConfdataFirstKeyType::one_dot_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc."), ConfdataFirstKeyType::one_dot_wildcard);

  ASSERT_EQ(w.detect_first_key_type(".."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc.."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type(".abc."), ConfdataFirstKeyType::two_dots_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc.def."), ConfdataFirstKeyType::two_dots_wildcard);

  ASSERT_EQ(w.detect_first_key_type("a"), ConfdataFirstKeyType::predefined_wildcard);
  ASSERT_EQ(w.detect_first_key_type("ab"), ConfdataFirstKeyType::predefined_wildcard);
  ASSERT_EQ(w.detect_first_key_type("abc"), ConfdataFirstKeyType::predefined_wildcard);
  ASSERT_EQ(w.detect_first_key_type("c"), ConfdataFirstKeyType::predefined_wildcard);

  ASSERT_FALSE(w.has_wildcard_for_key(""));
  ASSERT_FALSE(w.has_wildcard_for_key("."));
  ASSERT_FALSE(w.has_wildcard_for_key("b"));
  ASSERT_FALSE(w.has_wildcard_for_key("ba"));

  ASSERT_TRUE(w.has_wildcard_for_key("ab"));
  ASSERT_TRUE(w.has_wildcard_for_key("abc"));
  ASSERT_TRUE(w.has_wildcard_for_key("abc."));
  ASSERT_TRUE(w.has_wildcard_for_key("abc.def"));
  ASSERT_TRUE(w.has_wildcard_for_key("abc.def."));
  ASSERT_TRUE(w.has_wildcard_for_key("c.ab"));
  ASSERT_TRUE(w.has_wildcard_for_key("a.bc"));

  ASSERT_TRUE(w.has_wildcard_for_key("a"));
  ASSERT_TRUE(w.has_wildcard_for_key("c"));
  ASSERT_TRUE(w.has_wildcard_for_key("cxyz"));

  ASSERT_TRUE(w.is_most_common_predefined_wildcard("a"));
  ASSERT_FALSE(w.is_most_common_predefined_wildcard("ab"));
  ASSERT_FALSE(w.is_most_common_predefined_wildcard("abc"));
  ASSERT_TRUE(w.is_most_common_predefined_wildcard("c"));
}
