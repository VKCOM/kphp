// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/allocators/freelist.h"

#include <stdint.h>

#include <gtest/gtest.h>

TEST(freelist, basic) {
  {
    freelist_t freelist;
    freelist_init(&freelist);

    EXPECT_EQ(freelist_get(&freelist), nullptr);
  }

  {
    freelist_t freelist;
    freelist_init(&freelist);

    uint64_t dummy;
    freelist_put(&freelist, &dummy);
    EXPECT_EQ(freelist_get(&freelist), &dummy);
    EXPECT_EQ(freelist_get(&freelist), nullptr);
  }
}
