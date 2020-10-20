#include "common/smart_ptrs/tagged-ptr.h"

#include <limits>

#include <gtest/gtest.h>

TEST(tagged_ptr, basic) {
  char a;
  const uint16_t tag = 42;

  tagged_ptr_t tagged_ptr;
  tagged_ptr_pack(&tagged_ptr, &a, tag);

  EXPECT_EQ(tagged_ptr_get_tag(&tagged_ptr), tag);
  EXPECT_EQ(tagged_ptr_get_ptr(&tagged_ptr), &a);
}

TEST(tagged_ptr, next_tag) {
  {
    const uint16_t tag = 42;

    tagged_ptr_t tagged_ptr;
    tagged_ptr_pack(&tagged_ptr, NULL, tag);
    EXPECT_EQ(tagged_ptr_get_next_tag(&tagged_ptr), tag + 1);
    EXPECT_EQ(tagged_ptr_get_next_tag(&tagged_ptr), tag + 1);
  }

  {
    const uint16_t tag = std::numeric_limits<uint16_t>::max();

    tagged_ptr_t tagged_ptr;
    tagged_ptr_pack(&tagged_ptr, NULL, tag);
    EXPECT_EQ(tagged_ptr_get_next_tag(&tagged_ptr), 0);
    EXPECT_EQ(tagged_ptr_get_next_tag(&tagged_ptr), 0);
  }
}
