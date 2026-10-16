// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include "runtime-common/core/memory-resource/stack-resource.h"

namespace {

constexpr size_t BUFFER_SIZE{64};

} // namespace

TEST(stack_resource_test, uninited_state) {
  memory_resource::stack_resource resource;

  ASSERT_EQ(resource.memory_begin(), nullptr);
}

TEST(stack_resource_test, single_allocation_exact_buffer_fit) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* mem{resource.allocate(BUFFER_SIZE)};

  ASSERT_EQ(mem, static_cast<void*>(buffer.data()));
  ASSERT_EQ(resource.allocate(1), nullptr);
}

TEST(stack_resource_test, sequential_allocations_bump_pointer) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* a{resource.allocate(16)};
  void* b{resource.allocate(16)};
  void* c{resource.allocate(32)};

  ASSERT_EQ(a, static_cast<void*>(buffer.data()));
  ASSERT_EQ(b, static_cast<void*>(buffer.data() + 16));
  ASSERT_EQ(c, static_cast<void*>(buffer.data() + 32));
  ASSERT_EQ(resource.allocate(1), nullptr);
}

TEST(stack_resource_test, allocation_exceeding_remaining_capacity_returns_null_without_corrupting_state) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* mem{resource.allocate(BUFFER_SIZE - 16)};

  ASSERT_NE(mem, nullptr);
  ASSERT_EQ(resource.allocate(32), nullptr);

  void* rest{resource.allocate(16)};

  ASSERT_EQ(rest, static_cast<void*>(buffer.data() + (BUFFER_SIZE - 16)));
  ASSERT_EQ(resource.allocate(1), nullptr);
}

TEST(stack_resource_test, lifo_deallocate_reuses_address) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  resource.allocate(16);
  void* b1{resource.allocate(16)};

  resource.deallocate(b1, 16);
  void* b2{resource.allocate(16)};

  ASSERT_EQ(b2, b1);
}

TEST(stack_resource_test, multiple_lifo_deallocations_restore_cursor_in_order) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* a{resource.allocate(16)};
  void* b{resource.allocate(16)};
  void* c{resource.allocate(16)};

  resource.deallocate(c, 16);
  resource.deallocate(b, 16);

  void* b_again{resource.allocate(16)};
  void* c_again{resource.allocate(16)};

  ASSERT_EQ(b_again, b);
  ASSERT_EQ(c_again, c);

  resource.deallocate(c_again, 16);
  resource.deallocate(b_again, 16);
  resource.deallocate(a, 16);

  ASSERT_EQ(resource.allocate(BUFFER_SIZE), static_cast<void*>(buffer.data()));
}

TEST(stack_resource_test, allocate0_returns_zeroed_memory) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* mem{resource.allocate(BUFFER_SIZE)};
  ASSERT_NE(mem, nullptr);
  std::memset(mem, 0xAB, BUFFER_SIZE);
  resource.deallocate(mem, BUFFER_SIZE);

  void* mem0{resource.allocate0(BUFFER_SIZE)};

  ASSERT_EQ(mem0, mem);

  auto* bytes{static_cast<std::byte*>(mem0)};
  for (size_t i = 0; i < BUFFER_SIZE; ++i) {
    ASSERT_EQ(static_cast<unsigned char>(bytes[i]), 0);
  }
}

TEST(stack_resource_test, allocate0_on_full_buffer_returns_null) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  ASSERT_NE(resource.allocate(BUFFER_SIZE), nullptr);
  ASSERT_EQ(resource.allocate0(1), nullptr);
}

TEST(stack_resource_test, allocations_do_not_overlap) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  void* a{resource.allocate(20)};
  void* b{resource.allocate(20)};
  void* c{resource.allocate(20)};

  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(c, nullptr);

  std::memset(a, 1, 20);
  std::memset(b, 2, 20);
  std::memset(c, 2, 20); // intentionally same fill as b, to catch an overlap via pointer identity instead

  ASSERT_NE(a, b);
  ASSERT_NE(b, c);
  ASSERT_EQ(static_cast<std::byte*>(b) - static_cast<std::byte*>(a), 20);
  ASSERT_EQ(static_cast<std::byte*>(c) - static_cast<std::byte*>(b), 20);
}

TEST(stack_resource_test, memory_begin_returns_initial_buffer_and_is_stable) {
  std::vector<std::byte> buffer(BUFFER_SIZE);
  memory_resource::stack_resource resource;
  resource.init(buffer.data(), buffer.size());

  ASSERT_EQ(resource.memory_begin(), static_cast<void*>(buffer.data()));

  void* mem{resource.allocate(16)};
  ASSERT_EQ(resource.memory_begin(), static_cast<void*>(buffer.data()));

  resource.deallocate(mem, 16);
  ASSERT_EQ(resource.memory_begin(), static_cast<void*>(buffer.data()));
}
