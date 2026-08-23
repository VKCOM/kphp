// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include "runtime-common/core/memory-resource/chunk-pool-resource.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"

namespace {

constexpr size_t SEGMENT_SIZE{64};
// mirrors segment_list_node's layout: two pointers, no padding
constexpr size_t SEGMENT_HEADER_SIZE{2 * sizeof(void*)};
// mirrors chunk_pool_resource's internal buffer_list_node layout: a single pointer, no padding
constexpr size_t POOL_HEADER_SIZE{sizeof(void*)};
// size of one chunk as seen by the underlying SegmentPool: segment payload + segment_list_node header
constexpr size_t SEGMENT_CHUNK_SIZE{SEGMENT_SIZE + SEGMENT_HEADER_SIZE};

// heap-allocated (as opposed to std::array) to avoid -Warray-bounds false positives in gcc
[[gnu::noinline]] auto make_buffer(size_t segments) -> std::vector<std::byte> {
  return std::vector<std::byte>(POOL_HEADER_SIZE + segments * SEGMENT_CHUNK_SIZE);
}

auto segment_payload(std::byte* buffer_start, size_t index) noexcept -> std::byte* {
  return buffer_start + POOL_HEADER_SIZE + index * SEGMENT_CHUNK_SIZE + SEGMENT_HEADER_SIZE;
}

} // namespace

TEST(segmented_stack_resource_test, uninited_state) {
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;

  ASSERT_EQ(resource.allocate(1), nullptr);
  ASSERT_EQ(resource.allocate0(1), nullptr);
  ASSERT_EQ(resource.get_buffer_list_head(), nullptr);
}

TEST(segmented_stack_resource_test, single_allocation_exact_segment_fit) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* mem{resource.allocate(SEGMENT_SIZE)};

  ASSERT_EQ(mem, static_cast<void*>(segment_payload(buffer.data(), 0)));
  // the single segment is fully consumed, and there's no more pool memory for a new one
  ASSERT_EQ(resource.allocate(1), nullptr);
}

TEST(segmented_stack_resource_test, sequential_allocations_bump_within_segment) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* a{resource.allocate(16)};
  void* b{resource.allocate(16)};
  void* c{resource.allocate(32)};

  ASSERT_EQ(a, static_cast<void*>(segment_payload(buffer.data(), 0)));
  ASSERT_EQ(b, static_cast<void*>(segment_payload(buffer.data(), 0) + 16));
  ASSERT_EQ(c, static_cast<void*>(segment_payload(buffer.data(), 0) + 32));
  // exactly filled the segment
  ASSERT_EQ(resource.allocate(1), nullptr);
}

TEST(segmented_stack_resource_test, allocation_crossing_segment_boundary_switches_segment) {
  auto buffer{make_buffer(2)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* first{resource.allocate(SEGMENT_SIZE)};

  ASSERT_EQ(first, static_cast<void*>(segment_payload(buffer.data(), 0)));

  // doesn't fit in the remaining space of the first segment, must switch to a new one
  void* second{resource.allocate(1)};

  ASSERT_EQ(second, static_cast<void*>(segment_payload(buffer.data(), 1)));

  ASSERT_EQ(resource.allocate(SEGMENT_SIZE), nullptr); // pool exhausted
}

TEST(segmented_stack_resource_test, lifo_deallocate_reuses_address_within_segment) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  resource.allocate(16);
  void* b1{resource.allocate(16)};

  resource.deallocate(b1, 16);
  void* b2{resource.allocate(16)};

  ASSERT_EQ(b2, b1);
}

TEST(segmented_stack_resource_test, full_drain_returns_segment_to_pool_and_resets_state) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* mem1{resource.allocate(SEGMENT_SIZE)};

  ASSERT_NE(mem1, nullptr);

  resource.deallocate(mem1, SEGMENT_SIZE);

  // the segment was released back to the pool, so a fresh allocation gets the same address
  void* mem2{resource.allocate(SEGMENT_SIZE)};

  ASSERT_EQ(mem2, mem1);
}

TEST(segmented_stack_resource_test, regrowth_after_full_drain_across_multiple_segments) {
  auto buffer{make_buffer(2)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* first{resource.allocate(SEGMENT_SIZE)};
  void* second{resource.allocate(SEGMENT_SIZE)};

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  // unwind in LIFO order
  resource.deallocate(second, SEGMENT_SIZE);
  resource.deallocate(first, SEGMENT_SIZE);

  void* first_again{resource.allocate(SEGMENT_SIZE)};
  void* second_again{resource.allocate(SEGMENT_SIZE)};

  ASSERT_EQ(first_again, first);
  ASSERT_EQ(second_again, second);
}

TEST(segmented_stack_resource_test, multi_segment_unwind_restores_saved_cursor) {
  auto buffer{make_buffer(2)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* a{resource.allocate(48)}; // segment 0, [0, 48)
  void* b{resource.allocate(32)}; // doesn't fit remaining 16 bytes of segment 0, switches to segment 1
  void* c{resource.allocate(16)}; // segment 1, right after b

  ASSERT_EQ(a, static_cast<void*>(segment_payload(buffer.data(), 0)));
  ASSERT_EQ(b, static_cast<void*>(segment_payload(buffer.data(), 1)));
  ASSERT_EQ(c, static_cast<void*>(segment_payload(buffer.data(), 1) + 32));

  resource.deallocate(c, 16);
  resource.deallocate(b, 32); // drains segment 1 back to the pool

  // segment 0's cursor (saved when we switched away from it) must be restored correctly
  void* d{resource.allocate(16)};
  ASSERT_EQ(d, static_cast<void*>(segment_payload(buffer.data(), 0) + 48));
}

TEST(segmented_stack_resource_test, exhaustion_returns_null_without_corrupting_state) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* mem{resource.allocate(SEGMENT_SIZE - 16)};

  ASSERT_NE(mem, nullptr);

  // doesn't fit in the remaining 16 bytes, and there's no more pool memory for a new segment
  ASSERT_EQ(resource.allocate(32), nullptr);

  // the failed attempt must not have corrupted the cursor of the current segment
  void* rest{resource.allocate(16)};

  ASSERT_EQ(rest, static_cast<void*>(segment_payload(buffer.data(), 0) + (SEGMENT_SIZE - 16)));
}

TEST(segmented_stack_resource_test, allocate0_returns_zeroed_memory) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  void* mem{resource.allocate(SEGMENT_SIZE)};

  ASSERT_NE(mem, nullptr);

  resource.deallocate(mem, SEGMENT_SIZE);

  void* mem0{resource.allocate0(SEGMENT_SIZE)};

  ASSERT_EQ(mem0, mem);

  auto* bytes{static_cast<std::byte*>(mem0)};
  for (size_t i = 0; i < SEGMENT_SIZE; ++i) {
    ASSERT_EQ(static_cast<unsigned char>(bytes[i]), 0);
  }
}

TEST(segmented_stack_resource_test, allocate0_on_exhausted_pool_returns_null) {
  auto buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  ASSERT_NE(resource.allocate(SEGMENT_SIZE), nullptr);
  ASSERT_EQ(resource.allocate0(1), nullptr);
}

TEST(segmented_stack_resource_test, segments_do_not_overlap) {
  constexpr size_t SEGMENTS{4};
  auto buffer{make_buffer(SEGMENTS)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer.data(), buffer.size(), SEGMENT_SIZE);

  std::array<void*, SEGMENTS> segments{};
  for (size_t i = 0; i < SEGMENTS; ++i) {
    segments[i] = resource.allocate(SEGMENT_SIZE);

    ASSERT_NE(segments[i], nullptr);

    std::memset(segments[i], static_cast<int>(i + 1), SEGMENT_SIZE);
  }

  for (size_t i = 0; i < SEGMENTS; ++i) {
    auto* bytes{static_cast<std::byte*>(segments[i])};
    for (size_t j = 0; j < SEGMENT_SIZE; ++j) {
      ASSERT_EQ(bytes[j], static_cast<std::byte>(i + 1));
    }
  }
}

TEST(segmented_stack_resource_test, add_extra_memory_grows_pool_after_exhaustion) {
  auto buffer1{make_buffer(1)};
  auto buffer2{make_buffer(2)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer1.data(), buffer1.size(), SEGMENT_SIZE);

  ASSERT_NE(resource.allocate(SEGMENT_SIZE), nullptr);
  ASSERT_EQ(resource.allocate(1), nullptr); // exhausted

  resource.add_extra_memory(buffer2.data(), buffer2.size());

  ASSERT_NE(resource.allocate(SEGMENT_SIZE), nullptr);
  ASSERT_NE(resource.allocate(SEGMENT_SIZE), nullptr);
  ASSERT_EQ(resource.allocate(1), nullptr); // exhausted again
}

TEST(segmented_stack_resource_test, add_extra_memory_before_exhaustion_preserves_existing_free_segment) {
  auto old_buffer{make_buffer(2)};
  auto extra_buffer{make_buffer(1)};
  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(old_buffer.data(), old_buffer.size(), SEGMENT_SIZE);

  // consume the first segment of old_buffer; the second one is left free in the pool, not yet exhausted
  ASSERT_NE(resource.allocate(SEGMENT_SIZE), nullptr);

  void* remaining_old_segment{static_cast<void*>(segment_payload(old_buffer.data(), 1))};

  resource.add_extra_memory(extra_buffer.data(), extra_buffer.size());

  // the newly added buffer's segment is handed out before old_buffer's still-unused leftover segment
  ASSERT_EQ(resource.allocate(SEGMENT_SIZE), static_cast<void*>(segment_payload(extra_buffer.data(), 0)));
  ASSERT_EQ(resource.allocate(SEGMENT_SIZE), remaining_old_segment);
  ASSERT_EQ(resource.allocate(1), nullptr); // exhausted
}

TEST(segmented_stack_resource_test, buffer_list_head_tracks_most_recently_added_buffer) {
  auto buffer1{make_buffer(1)};
  auto buffer2{make_buffer(1)};

  memory_resource::segmented_stack_resource<memory_resource::chunk_pool_resource> resource;
  resource.init(buffer1.data(), buffer1.size(), SEGMENT_SIZE);

  ASSERT_EQ(static_cast<void*>(resource.get_buffer_list_head()), static_cast<void*>(buffer1.data()));

  resource.add_extra_memory(buffer2.data(), buffer2.size());

  ASSERT_EQ(static_cast<void*>(resource.get_buffer_list_head()), static_cast<void*>(buffer2.data()));

  auto* head{resource.get_buffer_list_head()};

  ASSERT_EQ(static_cast<void*>(head->next_in_chain), static_cast<void*>(buffer1.data()));

  head = head->next_in_chain;

  ASSERT_EQ(static_cast<void*>(head->next_in_chain), nullptr);
}
