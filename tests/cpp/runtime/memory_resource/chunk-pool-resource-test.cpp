// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <cstddef>
#include <cstring>

#include <gtest/gtest.h>

#include "runtime-common/core/memory-resource/chunk-pool-resource.h"

namespace {

constexpr size_t CHUNK_SIZE{64};
// mirrors chunk_pool_resource's internal list_node layout: a single pointer, no padding
constexpr size_t HEADER_SIZE{sizeof(void*)};

template<size_t N>
using aligned_buffer = std::array<std::byte, N>;

} // namespace

TEST(chunk_pool_resource_test, uninited_state) {
  memory_resource::chunk_pool_resource resource;

  ASSERT_EQ(resource.allocate(), nullptr);
  ASSERT_EQ(resource.allocate0(), nullptr);
  ASSERT_EQ(resource.get_buffer_list_head(), nullptr);
}

TEST(chunk_pool_resource_test, single_chunk_exact_buffer) {
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  void* mem{resource.allocate()};

  ASSERT_EQ(mem, static_cast<void*>(buffer.data() + HEADER_SIZE));
  ASSERT_EQ(resource.allocate(), nullptr);
}

TEST(chunk_pool_resource_test, multiple_chunks_exact_fit) {
  constexpr size_t CHUNKS{5};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNKS * CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  for (size_t i = 0; i < CHUNKS; ++i) {
    void* mem{resource.allocate()};
    ASSERT_EQ(mem, static_cast<void*>(buffer.data() + HEADER_SIZE + i * CHUNK_SIZE));
  }

  ASSERT_EQ(resource.allocate(), nullptr);
}

TEST(chunk_pool_resource_test, truncated_tail_is_unused) {
  constexpr size_t CHUNKS{3};
  constexpr size_t TAIL{CHUNK_SIZE - 1}; // smaller than one chunk, must be dropped
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNKS * CHUNK_SIZE + TAIL> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  for (size_t i = 0; i < CHUNKS; ++i) {
    ASSERT_NE(resource.allocate(), nullptr);
  }

  // the trailing TAIL bytes don't make up a whole chunk, so they must not be handed out
  ASSERT_EQ(resource.allocate(), nullptr);
}

TEST(chunk_pool_resource_test, minimal_chunk_size) {
  constexpr size_t MIN_CHUNK_SIZE{sizeof(void*)};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + 2 * MIN_CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), MIN_CHUNK_SIZE);

  void* a{resource.allocate()};
  void* b{resource.allocate()};

  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(a, b);
  ASSERT_EQ(resource.allocate(), nullptr);
}

TEST(chunk_pool_resource_test, allocate0_returns_zeroed_memory) {
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  void* mem{resource.allocate()};

  ASSERT_NE(mem, nullptr);

  std::memset(mem, 0xFF, CHUNK_SIZE);
  resource.deallocate(mem);
  void* mem0{resource.allocate0()};

  ASSERT_EQ(mem0, mem);

  auto* bytes{static_cast<std::byte*>(mem0)};
  for (size_t i = 0; i < CHUNK_SIZE; ++i) {
    ASSERT_EQ(static_cast<unsigned char>(bytes[i]), 0);
  }
}

TEST(chunk_pool_resource_test, allocate0_on_exhausted_pool_returns_null) {
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  ASSERT_NE(resource.allocate(), nullptr);
  ASSERT_EQ(resource.allocate0(), nullptr);
}

TEST(chunk_pool_resource_test, chunks_do_not_overlap) {
  constexpr size_t CHUNKS{4};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNKS * CHUNK_SIZE> buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer.data(), buffer.size(), CHUNK_SIZE);

  std::array<void*, CHUNKS> chunks{};
  for (size_t i = 0; i < CHUNKS; ++i) {
    chunks[i] = resource.allocate();

    ASSERT_NE(chunks[i], nullptr);

    std::memset(chunks[i], static_cast<int>(i + 1), CHUNK_SIZE);
  }

  for (size_t i = 0; i < CHUNKS; ++i) {
    auto* bytes{static_cast<std::byte*>(chunks[i])};
    for (size_t j = 0; j < CHUNK_SIZE; ++j) {
      ASSERT_EQ(bytes[j], static_cast<std::byte>(i + 1));
    }
  }
}

TEST(chunk_pool_resource_test, add_extra_memory_grows_pool_after_exhaustion) {
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer1{};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + 2 * CHUNK_SIZE> buffer2{};
  memory_resource::chunk_pool_resource resource;
  resource.init(buffer1.data(), buffer1.size(), CHUNK_SIZE);

  ASSERT_NE(resource.allocate(), nullptr);
  ASSERT_EQ(resource.allocate(), nullptr); // exhausted

  resource.add_extra_memory(buffer2.data(), buffer2.size());

  ASSERT_NE(resource.allocate(), nullptr);
  ASSERT_NE(resource.allocate(), nullptr);
  ASSERT_EQ(resource.allocate(), nullptr); // exhausted again
}

TEST(chunk_pool_resource_test, add_extra_memory_preserves_existing_free_chunks) {
  constexpr size_t OLD_CHUNKS{2};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + OLD_CHUNKS * CHUNK_SIZE> old_buffer{};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> extra_buffer{};
  memory_resource::chunk_pool_resource resource;
  resource.init(old_buffer.data(), old_buffer.size(), CHUNK_SIZE);

  // consume the first chunk of old_buffer, the second one is left in the free list
  ASSERT_NE(resource.allocate(), nullptr);

  void* remaining_old_chunk{static_cast<void*>(old_buffer.data() + HEADER_SIZE + CHUNK_SIZE)};
  resource.add_extra_memory(extra_buffer.data(), extra_buffer.size());

  // the newly added buffer's chunk is handed out before the old buffer's leftover chunk
  ASSERT_EQ(resource.allocate(), static_cast<void*>(extra_buffer.data() + HEADER_SIZE));
  ASSERT_EQ(resource.allocate(), remaining_old_chunk);
  ASSERT_EQ(resource.allocate(), nullptr);
}

TEST(chunk_pool_resource_test, buffer_list_head_tracks_most_recently_added_buffer) {
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer1{};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer2{};
  alignas(alignof(void*)) aligned_buffer<HEADER_SIZE + CHUNK_SIZE> buffer3{};

  memory_resource::chunk_pool_resource resource;

  resource.init(buffer1.data(), buffer1.size(), CHUNK_SIZE);

  ASSERT_EQ(static_cast<void*>(resource.get_buffer_list_head()), static_cast<void*>(buffer1.data()));

  resource.add_extra_memory(buffer2.data(), buffer2.size());

  ASSERT_EQ(static_cast<void*>(resource.get_buffer_list_head()), static_cast<void*>(buffer2.data()));

  resource.add_extra_memory(buffer3.data(), buffer3.size());

  ASSERT_EQ(static_cast<void*>(resource.get_buffer_list_head()), static_cast<void*>(buffer3.data()));

  auto* head{resource.get_buffer_list_head()};

  ASSERT_EQ(static_cast<void*>(head->next_in_chain), static_cast<void*>(buffer2.data()));

  head = head->next_in_chain;

  ASSERT_EQ(static_cast<void*>(head->next_in_chain), static_cast<void*>(buffer1.data()));

  head = head->next_in_chain;

  ASSERT_EQ(static_cast<void*>(head->next_in_chain), nullptr);
}
