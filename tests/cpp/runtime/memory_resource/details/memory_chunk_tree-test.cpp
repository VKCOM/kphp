#include <algorithm>
#include <gtest/gtest.h>
#include <random>

#include "kphp-core/memory_resource/details/memory_chunk_tree.h"
#include "kphp-core/memory_resource/details/memory_ordered_chunk_list.h"
#include "tests/cpp/runtime/memory_resource/details/test-helpers.h"

TEST(memory_chunk_tree_test, empty) {
  memory_resource::details::memory_chunk_tree mem_chunk_tree;
  ASSERT_FALSE(mem_chunk_tree.extract_smallest());
  ASSERT_FALSE(mem_chunk_tree.extract(1));
  ASSERT_FALSE(mem_chunk_tree.extract(9999));

  memory_resource::details::memory_ordered_chunk_list mem_list{nullptr};
  mem_chunk_tree.flush_to(mem_list);
  ASSERT_FALSE(mem_list.flush());
}

TEST(memory_chunk_tree_test, hard_reset) {
  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  char some_memory[1024];
  mem_chunk_tree.insert(some_memory, 100);
  mem_chunk_tree.insert(some_memory + 200, 150);

  mem_chunk_tree.hard_reset();
  ASSERT_FALSE(mem_chunk_tree.extract_smallest());
  ASSERT_FALSE(mem_chunk_tree.extract(1));
}

TEST(memory_chunk_tree_test, insert_extract) {
  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  char some_memory[1024 * 1024];
  const auto chunk_sizes = prepare_test_sizes();
  const auto chunk_offsets = make_offsets(chunk_sizes);

  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    mem_chunk_tree.insert(some_memory + chunk_offsets[i], chunk_sizes[i]);
  }

  size_t extracted = 0;
  for (int i = 0; i < 2; ++i, ++extracted) {
    auto *mem = mem_chunk_tree.extract(99);
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem),
              memory_resource::details::align_for_chunk(99));
  }

  for (int i = 0; i < 3; ++i, ++extracted) {
    auto *mem = mem_chunk_tree.extract(99);
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem),
              memory_resource::details::align_for_chunk(100));
  }

  for (int i = 0; i < 2; ++i, ++extracted) {
    auto *mem = mem_chunk_tree.extract(112);
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem),
              memory_resource::details::align_for_chunk(112));
  }

  auto *mem = mem_chunk_tree.extract(250);
  ASSERT_FALSE(mem);

  mem = mem_chunk_tree.extract(300);
  ASSERT_FALSE(mem);

  mem = mem_chunk_tree.extract(1);
  ++extracted;
  ASSERT_TRUE(mem);
  ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem),
            memory_resource::details::align_for_chunk(42));

  mem = mem_chunk_tree.extract(42);
  ++extracted;
  ASSERT_TRUE(mem);
  ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem),
            memory_resource::details::align_for_chunk(45));

  size_t prev_size = 0;
  for (; extracted != chunk_sizes.size(); ++extracted) {
    mem = mem_chunk_tree.extract(40);
    ASSERT_TRUE(mem);
    ASSERT_GE(memory_resource::details::memory_chunk_tree::get_chunk_size(mem), prev_size);
    prev_size = memory_resource::details::memory_chunk_tree::get_chunk_size(mem);
  }

  mem = mem_chunk_tree.extract(1);
  ASSERT_FALSE(mem);
}


TEST(memory_chunk_tree_test, insert_extract_smallest) {
  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  char some_memory[1024 * 1024];
  auto chunk_sizes = prepare_test_sizes();
  const auto chunk_offsets = make_offsets(chunk_sizes);

  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    mem_chunk_tree.insert(some_memory + chunk_offsets[i], chunk_sizes[i]);
  }

  std::sort(chunk_sizes.begin(), chunk_sizes.end());
  for (auto chunk_size: chunk_sizes) {
    auto *mem = mem_chunk_tree.extract_smallest();
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem), chunk_size);
  }

  auto *mem = mem_chunk_tree.extract_smallest();
  ASSERT_FALSE(mem);
}

TEST(memory_chunk_tree_test, flush_to) {
  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  char some_memory[1024 * 1024];
  const auto chunk_sizes = prepare_test_sizes();
  const auto chunk_offsets = make_offsets(chunk_sizes);

  for (size_t i = 0; i < chunk_sizes.size(); ++i) {
    mem_chunk_tree.insert(some_memory + chunk_offsets[i], chunk_sizes[i]);
  }

  memory_resource::details::memory_ordered_chunk_list ordered_list{some_memory};
  mem_chunk_tree.flush_to(ordered_list);
  ASSERT_FALSE(mem_chunk_tree.extract_smallest());

  auto *first_node = ordered_list.flush();
  ASSERT_TRUE(first_node);
  ASSERT_EQ(first_node->size(), chunk_offsets.back());
  ASSERT_EQ(reinterpret_cast<char *>(first_node), some_memory);

  ASSERT_FALSE(ordered_list.get_next(first_node));
}

TEST(memory_chunk_tree_test, random_insert) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dis(48, 512);

  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  constexpr size_t total_chunks = 16 * 1024;
  std::array<size_t, total_chunks> sizes{0};
  for (auto &size : sizes) {
    size = dis(gen);
    mem_chunk_tree.insert(std::malloc(size), size);
  }

  std::shuffle(sizes.begin(), sizes.end(), gen);
  for (auto size : sizes) {
    auto *mem = mem_chunk_tree.extract(size);
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem), size);
    std::free(mem);
  }
  ASSERT_FALSE(mem_chunk_tree.extract(0));
}

TEST(memory_chunk_tree_test, random_extract_smallest) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dis(48, 512);

  memory_resource::details::memory_chunk_tree mem_chunk_tree;

  constexpr size_t total_chunks = 16 * 1024;
  std::array<size_t, total_chunks> sizes{0};
  for (auto &size : sizes) {
    size = dis(gen);
    mem_chunk_tree.insert(std::malloc(size), size);
  }

  std::sort(sizes.begin(), sizes.end());
  for (auto size : sizes) {
    auto *mem = mem_chunk_tree.extract_smallest();
    ASSERT_TRUE(mem);
    ASSERT_EQ(memory_resource::details::memory_chunk_tree::get_chunk_size(mem), size);
    std::free(mem);
  }
  ASSERT_FALSE(mem_chunk_tree.extract_smallest());
}
