// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <unordered_set>

#include "common/algorithms/switch_hash.h"
#include "common/algorithms/contains.h"

TEST(test_switch_hash, zero_hashes) {
  std::vector<std::string> strings = {
    "this string is too long",
    {"null\0", 5},
    "",
  };
  for (const auto &s : strings) {
    ASSERT_EQ(vk::case_hash8(s.data(), s.size()), 0);
  }
}

TEST(test_switch_hash, collisions) {
  // test that all hashes produced for these strings are unique
  std::vector<std::string> strings = {
    "foo",
    "fo",
    "f",
    "oof",
    "of",
    "ofo",
    "fooo",
    "ooof",
    "fff",
    "ooo",
    "a\xfa",
    "a\xfb",
    "b\xfa",
    "b\xfb",
    "a\x7f",
    "b\x7f",
  };
  std::unordered_set<uint64_t> hashes;
  for (const auto &s : strings) {
    uint64_t h = vk::case_hash8(s.data(), s.size());
    ASSERT_TRUE(!vk::contains(hashes, h));
    hashes.insert(h);
  }
}

TEST(test_switch_hash, single_char) {
  std::vector<const char*> chars = {
    "a",
    "A",
    "b",
    "Z",
    "4",
    "$",
    "@",
    "\n",
    "\xff",
    "\xfa",
  };
  for (const auto &ch : chars) {
    uint64_t have = vk::case_hash8(ch, 1);
    uint64_t have2 = vk::case_hash8<1>(ch, 1);
    uint64_t want = static_cast<unsigned char>(ch[0]);
    ASSERT_EQ(have, want);
    ASSERT_EQ(have2, want);
  }

  ASSERT_EQ(0, vk::case_hash8<1>("foo", 3));
  ASSERT_EQ(0, vk::case_hash8<1>("x\0", 2));
  ASSERT_EQ(0, vk::case_hash8<1>("\0", 1));
}

TEST(test_switch_hash, non_matching) {
  // the compiler ensures that switch statement cases don't have null bytes,
  // but the tag string (condition value) can contain null bytes nonetheless;
  // our hash function ensures that these hashes won't match
  std::vector<std::string> shouldnt_match = {
    {"\0", 1},
    {"\0\0\0", 3},
    {"\0foo", 4},
    {"f\0oo", 4},
    {"fo\0o", 4},
    {"fo\0", 3},
    {"fo\0\0o", 5},
    {"foo\0", 4},
    {"foo\0\0", 5},
    {"foo\0\0\0", 6},
    {"f\0o\0o", 5},
    {"\0\0foo", 5},
    {"\0\0\0foo", 6},
    "f00",
    "fooo",
    "foa",
    "fo0",
    "fao",
    "foa",
    "fo",
    "f",
    "",
  };
  uint64_t foo_hash = vk::case_hash8("foo", 3);
  for (const auto &s : shouldnt_match) {
    ASSERT_NE(foo_hash, vk::case_hash8(s.data(), s.size())) << "string size is " << s.size();
  }
}
