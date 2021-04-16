#include <gtest/gtest.h>

#include "runtime/memory_resource/extra-memory-pool.h"

namespace mr = memory_resource;

constexpr size_t pool_sizeof = 16;

TEST(extra_memory_pool_test, test_payload_size_member) {
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

TEST(extra_memory_pool_test, test_size_by_bucket_id) {
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(0), 1 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(1), 2 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(2), 4 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(3), 8 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(4), 16 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(5), 32 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(6), 64 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(7), 128 * 1024 * 1024);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_size_by_bucket(8), 256 * 1024 * 1024);
}

TEST(extra_memory_pool_test, test_bucket_id_by_size) {
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{1 * 1024 * 1024}), 0);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{2 * 1024 * 1024}), 1);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{4 * 1024 * 1024}), 2);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{8 * 1024 * 1024}), 3);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{pool_sizeof * 1024 * 1024}), 4);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{32 * 1024 * 1024}), 5);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{64 * 1024 * 1024}), 6);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{128 * 1024 * 1024}), 7);
  ASSERT_EQ(mr::extra_memory_raw_bucket::get_bucket(mr::extra_memory_pool{256 * 1024 * 1024}), 8);
}

TEST(extra_memory_pool_test, test_memory_distribution_not_enougth_memory) {
  constexpr size_t buffer_size = 512 * 1024;
  auto buffer = std::make_unique<uint8_t[]>(buffer_size);
  std::array<mr::extra_memory_raw_bucket, 7> extra_memory;
  ASSERT_EQ(mr::distribute_memory(extra_memory, 100, buffer.get(), buffer_size), buffer_size);
  for (auto &m : extra_memory) {
    ASSERT_EQ(m.buffers_count(), 0);
  }
}

TEST(extra_memory_pool_test, test_memory_distribution_left_some_memory) {
  constexpr size_t buffer_size = 5 * 1024 * 1024 + 123456;
  auto buffer = std::make_unique<uint8_t[]>(buffer_size);
  std::array<mr::extra_memory_raw_bucket, 7> extra_memory;
  ASSERT_EQ(mr::distribute_memory(extra_memory, 2, buffer.get(), buffer_size), 123456);

  ASSERT_EQ(extra_memory[0].buffers_count(), 3);
  ASSERT_EQ(extra_memory[0].get_extra_pool_raw(0), buffer.get());
  ASSERT_EQ(extra_memory[0].get_extra_pool_raw(1), buffer.get() + 1 * 1024 * 1024);
  ASSERT_EQ(extra_memory[0].get_extra_pool_raw(2), buffer.get() + 2 * 1024 * 1024);

  ASSERT_EQ(extra_memory[1].buffers_count(), 1);
  ASSERT_EQ(extra_memory[1].get_extra_pool_raw(0), buffer.get() + 3 * 1024 * 1024);

  for (size_t i = 2; i != extra_memory.size(); ++i) {
    ASSERT_EQ(extra_memory[i].buffers_count(), 0);
  }
}

TEST(extra_memory_pool_test, test_memory_distribution_more_than_required) {
  constexpr size_t buffer_size = 701 * 1024 * 1024 + 123456;
  std::array<mr::extra_memory_raw_bucket, 7> extra_memory;
  ASSERT_EQ(mr::distribute_memory(extra_memory, 2, nullptr, buffer_size), 123456);
  // 0 + 3*1 = 3MB
  ASSERT_EQ(extra_memory[0].buffers_count(), 3);
  ASSERT_EQ(extra_memory[0].get_extra_pool_raw(0), reinterpret_cast<void *>(0));
  // 3 + 1*2 = 5MB
  ASSERT_EQ(extra_memory[1].buffers_count(), 1);
  ASSERT_EQ(extra_memory[1].get_extra_pool_raw(0), reinterpret_cast<void *>(3 * 1024 * 1024));
  // 5 + 2*4 = 13MB
  ASSERT_EQ(extra_memory[2].buffers_count(), 2);
  ASSERT_EQ(extra_memory[2].get_extra_pool_raw(0), reinterpret_cast<void *>(5 * 1024 * 1024));
  // 13 + 2*8 = 29MB
  ASSERT_EQ(extra_memory[3].buffers_count(), 2);
  ASSERT_EQ(extra_memory[3].get_extra_pool_raw(0), reinterpret_cast<void *>(13 * 1024 * 1024));
  // 29 + 2*16 = 61MB
  ASSERT_EQ(extra_memory[4].buffers_count(), 2);
  ASSERT_EQ(extra_memory[4].get_extra_pool_raw(0), reinterpret_cast<void *>(29 * 1024 * 1024));
  // 61 + 2*32 = 125MB
  ASSERT_EQ(extra_memory[5].buffers_count(), 2);
  ASSERT_EQ(extra_memory[5].get_extra_pool_raw(0), reinterpret_cast<void *>(61 * 1024 * 1024));
  // 125 + 9*64 = 701MB
  ASSERT_EQ(extra_memory[6].buffers_count(), 9);
  ASSERT_EQ(extra_memory[6].get_extra_pool_raw(0), reinterpret_cast<void *>(125 * 1024 * 1024));
}

TEST(extra_memory_pool_test, test_memory_distribution) {
  constexpr size_t processes = 300;
  // 300*7 = 2100MB
  constexpr size_t buffer_size = processes * 7 * 1024 * 1024;
  std::array<mr::extra_memory_raw_bucket, 7> extra_memory;
  ASSERT_EQ(mr::distribute_memory(extra_memory, processes, nullptr, buffer_size), 0);
  // 0 + 300*1 = 300MB // left 1800MB
  ASSERT_EQ(extra_memory[0].buffers_count(), processes);
  ASSERT_EQ(extra_memory[0].get_extra_pool_raw(0), reinterpret_cast<void *>(0));
  // 300 + 150*2 = 600MB // left 1500MB
  ASSERT_EQ(extra_memory[1].buffers_count(), processes / 2);
  ASSERT_EQ(extra_memory[1].get_extra_pool_raw(0), reinterpret_cast<void *>(processes * 1024 * 1024));
  // 600 + 75*4 = 900MB // left 1200MB
  ASSERT_EQ(extra_memory[2].buffers_count(), processes / 4);
  ASSERT_EQ(extra_memory[2].get_extra_pool_raw(0), reinterpret_cast<void *>(processes * 2 * 1024 * 1024));
  // 900 + 38*8 = 1204MB // left 896MB
  ASSERT_EQ(extra_memory[3].buffers_count(), 1 + processes / 8);
  ASSERT_EQ(extra_memory[3].get_extra_pool_raw(0), reinterpret_cast<void *>(processes * 3 * 1024 * 1024));
  // 1204 + 18*16 = 1492MB // left 608MB
  ASSERT_EQ(extra_memory[4].buffers_count(), processes / 16);
  ASSERT_EQ(extra_memory[4].get_extra_pool_raw(0), reinterpret_cast<void *>((processes * 4 + 4) * 1024 * 1024));
  // 1492 + 9*32 = 1780MB // left 320MB
  ASSERT_EQ(extra_memory[5].buffers_count(), processes / 32);
  ASSERT_EQ(extra_memory[5].get_extra_pool_raw(0), reinterpret_cast<void *>((processes * 5 - 8) * 1024 * 1024));
  // 1780 + 5*64 = 2100MB
  ASSERT_EQ(extra_memory[6].buffers_count(), 1 + processes / 64);
  ASSERT_EQ(extra_memory[6].get_extra_pool_raw(0), reinterpret_cast<void *>((processes * 6 - 20) * 1024 * 1024));
}
