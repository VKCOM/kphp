#include <gtest/gtest.h>

#include "runtime-common/runtime-core/memory-resource/details/memory_chunk_list.h"

TEST(unsynchronized_memory_chunk_list_test, empty) {
  memory_resource::details::memory_chunk_list mem_chunk_list;
  ASSERT_EQ(mem_chunk_list.get_mem(), nullptr);
}

TEST(unsynchronized_memory_chunk_list_test, put_get_mem) {
  memory_resource::details::memory_chunk_list mem_chunk_list;

  uint64_t x, y, z;
  mem_chunk_list.put_mem(&x);
  mem_chunk_list.put_mem(&y);
  mem_chunk_list.put_mem(&z);

  ASSERT_EQ(mem_chunk_list.get_mem(), &z);
  ASSERT_EQ(mem_chunk_list.get_mem(), &y);
  ASSERT_EQ(mem_chunk_list.get_mem(), &x);

  ASSERT_EQ(mem_chunk_list.get_mem(), nullptr);
}
