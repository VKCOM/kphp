#include <algorithm>
#include <gtest/gtest.h>
#include <random>


#include "runtime-core/memory-resource/details/memory_ordered_chunk_list.h"
#include "runtime-core/memory-resource/monotonic_buffer_resource.h"
#include "tests/cpp/runtime/memory_resource/details/test-helpers.h"

TEST(memory_ordered_chunk_list_test, empty) {
  memory_resource::details::memory_ordered_chunk_list mem_list{nullptr};
  ASSERT_FALSE(mem_list.flush());
}

TEST(memory_ordered_chunk_list_test, add_memory_and_flush_to) {
  char some_memory[1024 * 1024];
  const auto chunk_sizes = prepare_test_sizes();
  const auto chunk_offsets = make_offsets(chunk_sizes);

  memory_resource::details::memory_ordered_chunk_list ordered_list{some_memory};
  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    ordered_list.add_memory(some_memory + chunk_offsets[i], chunk_sizes[i]);
  }

  constexpr size_t gap1 = memory_resource::details::align_for_chunk(510250);
  ASSERT_GT(gap1, chunk_offsets.back());
  char *some_memory_with_gap1 = some_memory + gap1;
  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    ordered_list.add_memory(some_memory_with_gap1 + chunk_offsets[i], chunk_sizes[i]);
  }

  constexpr size_t gap2 = memory_resource::details::align_for_chunk(235847);
  ASSERT_GT(gap2, chunk_offsets.back());
  ASSERT_GT(gap1, chunk_offsets.back() + gap2);
  char *some_memory_with_gap2 = some_memory + gap2;
  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    ordered_list.add_memory(some_memory_with_gap2 + chunk_offsets[i], chunk_sizes[i]);
  }

  auto *gap1_node = ordered_list.flush();
  ASSERT_TRUE(gap1_node);
  ASSERT_EQ(gap1_node->size(), chunk_offsets.back());
  ASSERT_EQ(reinterpret_cast<char *>(gap1_node), some_memory_with_gap1);

  auto *gap2_node = ordered_list.get_next(gap1_node);
  ASSERT_TRUE(gap2_node);
  ASSERT_EQ(gap2_node->size(), chunk_offsets.back());
  ASSERT_EQ(reinterpret_cast<char *>(gap2_node), some_memory_with_gap2);

  auto *no_gap_node = ordered_list.get_next(gap2_node);
  ASSERT_TRUE(no_gap_node);
  ASSERT_EQ(no_gap_node->size(), chunk_offsets.back());
  ASSERT_EQ(reinterpret_cast<char *>(no_gap_node), some_memory);

  ASSERT_FALSE(ordered_list.get_next(no_gap_node));
}

TEST(memory_ordered_chunk_list_test, add_random) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dis(8, 63);

  constexpr size_t memory_size = 1024 * 1024;
  char some_memory[memory_size];
  memory_resource::monotonic_buffer_resource mem_resource;
  mem_resource.init(some_memory, memory_size);

  constexpr size_t total_chunks = 16 * 1024;
  std::array<std::pair<char *, size_t>, total_chunks> chunks;

  memory_resource::details::memory_ordered_chunk_list ordered_list{some_memory};

  for (auto &chunk : chunks) {
    size_t mem_size = dis(gen);
    char *mem = static_cast<char *>(mem_resource.allocate(memory_resource::details::align_for_chunk(mem_size + 1)));
    ordered_list.add_memory(mem, mem_size);
    chunk = {mem, mem_size};
  }

  std::sort(chunks.begin(), chunks.end(), std::greater<>{});

  auto *mem_from_list = ordered_list.flush();
  for (auto chunk : chunks) {
    ASSERT_TRUE(mem_from_list);
    ASSERT_EQ(chunk.first, reinterpret_cast<char *>(mem_from_list));
    ASSERT_EQ(chunk.second, mem_from_list->size());
    mem_resource.deallocate(mem_from_list, mem_from_list->size() + 1);
    mem_from_list = ordered_list.get_next(mem_from_list);
  }
  ASSERT_FALSE(mem_from_list);
}
