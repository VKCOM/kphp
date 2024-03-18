#include <array>
#include <gtest/gtest.h>

#include "runtime/memory_resource/unsynchronized_pool_resource.h"

TEST(unsynchronized_pool_resource_test, uninited_state) {
  memory_resource::unsynchronized_pool_resource resource;

  auto mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, 0);
  ASSERT_EQ(mem_stats.max_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_limit, 0);
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  // no effect
  resource.perform_defragmentation();

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, 0);
  ASSERT_EQ(mem_stats.max_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_limit, 0);
  ASSERT_EQ(mem_stats.defragmentation_calls, 1);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);
}

TEST(unsynchronized_pool_resource_test, simple_allocation) {
  std::array<char, 1024 * 128> some_memory{};
  memory_resource::unsynchronized_pool_resource resource;

  resource.init(some_memory.data(), some_memory.size());

  auto mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, 0);
  ASSERT_EQ(mem_stats.max_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  void *mem1 = resource.allocate(1);
  ASSERT_TRUE(mem1);

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 8);
  ASSERT_EQ(mem_stats.memory_used, 8);
  ASSERT_EQ(mem_stats.max_real_memory_used, 8);
  ASSERT_EQ(mem_stats.max_memory_used, 8);
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  void *mem65530 = resource.allocate(65530);
  ASSERT_TRUE(mem65530);

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.max_real_memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.max_memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  resource.deallocate(mem65530, 65530);
  resource.deallocate(mem1, 1);

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.max_memory_used, 8 + 1024 * 64);
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);
}

TEST(unsynchronized_pool_resource_test, fallback_pool_with_defragmentation) {
  std::array<char, 131072> some_memory{}; // 1024*128
  memory_resource::unsynchronized_pool_resource resource;

  resource.init(some_memory.data(), some_memory.size());

  void *mem65536 = resource.allocate(65536);
  ASSERT_TRUE(mem65536);

  void *mem8 = resource.allocate(8);
  ASSERT_TRUE(mem8);

  void *mem_left = resource.allocate(some_memory.size() - 65536 - 8 - 8);
  ASSERT_TRUE(mem_left);

  void *mem8_2 = resource.allocate(8);
  ASSERT_TRUE(mem8_2);

  resource.deallocate(mem65536, 65536);
  resource.deallocate(mem8, 8);
  resource.deallocate(mem_left, some_memory.size() - 65536 - 8 - 8);
  resource.deallocate(mem8_2, 8);

  // got fragmentation
  auto mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size() - 8);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 2);
  ASSERT_EQ(mem_stats.small_memory_pieces, 1);

  // but we have 2 large piece with sizes 65536 - 8 - 8 and  65536, resource will use it
  int peace_left_size = 65536 - 8 - 8;
  void *mem32 = resource.allocate(32);
  ASSERT_TRUE(mem32);
  peace_left_size -= 32;

  void *mem64 = resource.allocate(64);
  ASSERT_TRUE(mem64);
  peace_left_size -= 64;

  void *mem128 = resource.allocate(128);
  ASSERT_TRUE(mem128);
  peace_left_size -= 128;

  void *mem16376_1 = resource.allocate(16376); // 16 *1024 - 8 => largest slot
  ASSERT_TRUE(mem16376_1);
  peace_left_size -= 16376;

  void *mem16376_2 = resource.allocate(16376); // 16 *1024 - 8 => largest slot
  ASSERT_TRUE(mem16376_2);
  peace_left_size -= 16376;

  void *mem16376_3 = resource.allocate(16376); // 16 *1024 - 8 => largest slot
  ASSERT_TRUE(mem16376_3);
  peace_left_size -= 16376;

  ASSERT_GT(peace_left_size, 0);
  ASSERT_LT(peace_left_size, 16376);

  // take other large piece, because previous one (65536 - 8 - 8) is over
  void *mem16376_4 = resource.allocate(16376); // 16 *1024 - 8 => largest slot
  ASSERT_TRUE(mem16376_4);

  mem_stats = resource.get_memory_stats();
  // after usage peace_left_size was returned into common buffer
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size() - 8 - peace_left_size);
  ASSERT_EQ(mem_stats.memory_used, 32 + 64 + 128 + 16376 * 4);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 1);

  resource.deallocate(mem32, 32);
  resource.deallocate(mem64, 64);
  resource.deallocate(mem128, 128);
  resource.deallocate(mem16376_1, 16376);
  resource.deallocate(mem16376_2, 16376);
  resource.deallocate(mem16376_3, 16376);
  resource.deallocate(mem16376_4, 16376);

  mem_stats = resource.get_memory_stats();
  // one peace was returned into common buffer
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size() - 8 - peace_left_size - 16376);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 7);

  // perform defragmentation - make resource great again!
  resource.perform_defragmentation();

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 0);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 1);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  void *all_mem = resource.allocate(some_memory.size());
  ASSERT_TRUE(all_mem);
}

TEST(unsynchronized_pool_resource_test, test_auto_defragmentation_huge_piece) {
  std::array<char, 131072> some_memory{}; // 1024*128
  memory_resource::unsynchronized_pool_resource resource;

  resource.init(some_memory.data(), some_memory.size());

  void *mem65536 = resource.allocate(65536);
  ASSERT_TRUE(mem65536);

  void *mem8 = resource.allocate(8);
  ASSERT_TRUE(mem8);

  void *mem_left = resource.allocate(some_memory.size() - 65536 - 8 - 8);
  ASSERT_TRUE(mem_left);

  void *mem8_2 = resource.allocate(8);
  ASSERT_TRUE(mem8_2);

  resource.deallocate(mem65536, 65536);
  resource.deallocate(mem8, 8);
  resource.deallocate(mem_left, some_memory.size() - 65536 - 8 - 8);
  resource.deallocate(mem8_2, 8);

  // got fragmentation
  auto mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size() - 8);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 2);
  ASSERT_EQ(mem_stats.small_memory_pieces, 1);

  // auto defragmentation
  void *mem_all = resource.allocate(some_memory.size());
  ASSERT_TRUE(mem8_2);

  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 1);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);
  resource.deallocate(mem_all, some_memory.size());
}

TEST(unsynchronized_pool_resource_test, test_auto_defragmentation_small_piece) {
  std::array<char, 1024 * 32> some_memory{};
  memory_resource::unsynchronized_pool_resource resource;

  resource.init(some_memory.data(), some_memory.size());

  std::array<void *, 1024> pieces32{};
  for (auto &mem : pieces32) {
    mem = resource.allocate(32);
  }

  for (auto &mem : pieces32) {
    resource.deallocate(mem, 32);
  }

  // got fragmentation
  auto mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, some_memory.size() - 32);
  ASSERT_EQ(mem_stats.memory_used, 0);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 0);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 1023);

  // auto defragmentation
  void *mem64 = resource.allocate(64);
  mem_stats = resource.get_memory_stats();
  ASSERT_EQ(mem_stats.real_memory_used, 64);
  ASSERT_EQ(mem_stats.memory_used, 64);
  ASSERT_EQ(mem_stats.max_real_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.max_memory_used, some_memory.size());
  ASSERT_EQ(mem_stats.memory_limit, some_memory.size());
  ASSERT_EQ(mem_stats.defragmentation_calls, 1);
  ASSERT_EQ(mem_stats.huge_memory_pieces, 0);
  ASSERT_EQ(mem_stats.small_memory_pieces, 0);

  resource.deallocate(mem64, 64);
}