// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>
#include <memory>

#include "runtime-common/runtime-core/memory-resource/extra-memory-pool.h"

namespace mr = memory_resource;

constexpr size_t pool_sizeof = 16;

TEST(extra_memory_pool_test, test_payload_size_member) {
  ASSERT_EQ(mr::extra_memory_pool{256 * 1024}.get_pool_payload_size(), 256 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{512 * 1024}.get_pool_payload_size(), 512 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{1 * 1024 * 1024}.get_pool_payload_size(), 1 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{2 * 1024 * 1024}.get_pool_payload_size(), 2 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{4 * 1024 * 1024}.get_pool_payload_size(), 4 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{8 * 1024 * 1024}.get_pool_payload_size(), 8 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{16 * 1024 * 1024}.get_pool_payload_size(), 16 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{32 * 1024 * 1024}.get_pool_payload_size(), 32 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{64 * 1024 * 1024}.get_pool_payload_size(), 64 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{128 * 1024 * 1024}.get_pool_payload_size(), 128 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool{256 * 1024 * 1024}.get_pool_payload_size(), 256 * 1024 * 1024 - pool_sizeof);
}

TEST(extra_memory_pool_test, test_payload_size_static_member) {
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(256 * 1024), 256 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(512 * 1024), 512 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(1 * 1024 * 1024), 1 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(2 * 1024 * 1024), 2 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(4 * 1024 * 1024), 4 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(8 * 1024 * 1024), 8 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(16 * 1024 * 1024), 16 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(32 * 1024 * 1024), 32 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(64 * 1024 * 1024), 64 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(128 * 1024 * 1024), 128 * 1024 * 1024 - pool_sizeof);
  ASSERT_EQ(mr::extra_memory_pool::get_pool_payload_size(256 * 1024 * 1024), 256 * 1024 * 1024 - pool_sizeof);
}

template<size_t N, uint8_t c>
void do_extra_memory_pool_test() {
  auto buffer = std::make_unique<uint8_t[]>(N);
  memset(buffer.get(), c, N);

  auto *extra_pool = new(buffer.get()) mr::extra_memory_pool{N};

  ASSERT_EQ(extra_pool->get_pool_payload_size(), N - pool_sizeof);
  ASSERT_EQ(extra_pool->get_buffer_size(), N);
  ASSERT_EQ(extra_pool->next_in_chain, nullptr);
  ASSERT_EQ(extra_pool->memory_begin(), buffer.get() + pool_sizeof);
  for (size_t i = 0; i != pool_sizeof; ++i) {
    ASSERT_NE(buffer[i], c);
  }
  for (size_t i = 0; i != extra_pool->get_pool_payload_size(); ++i) {
    ASSERT_EQ(buffer[pool_sizeof + i], c);
  }

  memset(extra_pool->memory_begin(), c + 1, extra_pool->get_pool_payload_size());
  ASSERT_EQ(extra_pool->get_pool_payload_size(), N - pool_sizeof);
  ASSERT_EQ(extra_pool->get_buffer_size(), N);
  ASSERT_EQ(extra_pool->next_in_chain, nullptr);
  ASSERT_EQ(extra_pool->memory_begin(), buffer.get() + pool_sizeof);
  for (size_t i = 0; i != pool_sizeof; ++i) {
    ASSERT_NE(buffer[i], c + 1);
  }
  for (size_t i = 0; i != extra_pool->get_pool_payload_size(); ++i) {
    ASSERT_EQ(buffer[pool_sizeof + i], c + 1);
  }
}

TEST(extra_memory_pool_test, test_extra_memory_pool) {
  do_extra_memory_pool_test<16, 0x5c>();
  do_extra_memory_pool_test<17, 0x6c>();
  do_extra_memory_pool_test<65, 0x7c>();
  do_extra_memory_pool_test<2365, 0x8c>();
  do_extra_memory_pool_test<56789, 0x9c>();

  do_extra_memory_pool_test<1 * 1024 * 1024, 0x0d>();
  do_extra_memory_pool_test<2 * 1024 * 1024, 0x1d>();
  do_extra_memory_pool_test<4 * 1024 * 1024, 0x2d>();
}
