// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-aes-keys.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include <gtest/gtest.h>

TEST(net_aes_keys, basic) {
  aes_key_t *key = create_aes_key();
  key->id = 0x42;
  key->filename = "area51";
  uint8_t *key_body = key->key;
  std::iota(&key->key[0], &key->key[AES_KEY_MAX_LEN], 0);

  EXPECT_TRUE(aes_key_add(key));
  EXPECT_EQ(find_aes_key_by_id(key->id), key);
  EXPECT_EQ(find_aes_key_by_filename(key->filename), key);

  auto first_not_zero = std::find_if_not(std::next(key_body), std::next(key_body, AES_KEY_MAX_LEN), [](uint8_t byte) { return byte != 0; });
  EXPECT_EQ(first_not_zero, std::next(key_body, AES_KEY_MAX_LEN));

  free_aes_key(key);
}
