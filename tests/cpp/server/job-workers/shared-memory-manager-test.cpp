// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>

#include "common/macos-ports.h"

#include "runtime/memory_resource/extra-memory-pool.h"
#include "server/job-workers/job-message.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/php-engine-vars.h"
#include "server/workers-control.h"

using namespace job_workers;
using SHMM = vk::singleton<SharedMemoryManager>;

TEST(shared_memory_manager_test, test_memory_limit) {
  ASSERT_FALSE(SHMM::get().set_memory_limit(0));
  ASSERT_FALSE(SHMM::get().set_memory_limit(10));
  ASSERT_FALSE(SHMM::get().set_memory_limit(15 * 1025));
  ASSERT_FALSE(SHMM::get().set_memory_limit(100500));

  ASSERT_TRUE(SHMM::get().set_memory_limit(1024 * 1024 * 4));
}

void check_new_message(JobSharedMessage *message) {
  ASSERT_NE(message, nullptr);
  ASSERT_EQ(message->owners_counter, 1);
  ASSERT_EQ(message->job_id, 0);
  ASSERT_EQ(message->job_result_fd_idx, -1);
  ASSERT_EQ(message->job_start_time, -1.0);
  ASSERT_EQ(message->job_timeout, -1.0);
}

struct AcquiredReleased {
  size_t acquired{};
  size_t released{};
};

#define ASSERT_ACQUIRED(stat, before)                                                                                                                          \
  {                                                                                                                                                            \
    const size_t new_stat = (stat).acquired;                                                                                                                   \
    ASSERT_GT(new_stat, before.acquired);                                                                                                                      \
    before.acquired = new_stat;                                                                                                                                \
  }

#define ASSERT_RELEASED(stat, before)                                                                                                                          \
  {                                                                                                                                                            \
    const size_t new_stat = (stat).released;                                                                                                                   \
    ASSERT_GT(new_stat, before.released);                                                                                                                      \
    before.released = new_stat;                                                                                                                                \
  }

void worker_function() {
  AcquiredReleased messages;
  AcquiredReleased _256kb;
  AcquiredReleased _512kb;
  AcquiredReleased _1mb;
  AcquiredReleased _2mb;
  AcquiredReleased _4mb;

  for (size_t i = 0; i != 10000; ++i) {
    auto *message = SHMM::get().acquire_shared_message<job_workers::JobSharedMessage>();
    ASSERT_ACQUIRED(SHMM::get().get_stats().messages, messages);
    check_new_message(message);
    message->job_id = i;
    SHMM::get().release_shared_message(message);
    ASSERT_RELEASED(SHMM::get().get_stats().messages, messages);

    auto *leaked_message = SHMM::get().acquire_shared_message<job_workers::JobSharedMessage>();
    ASSERT_ACQUIRED(SHMM::get().get_stats().messages, messages);
    check_new_message(leaked_message);
    leaked_message->job_id = i + 123124;
    SHMM::get().forcibly_release_all_attached_messages();
    ASSERT_RELEASED(SHMM::get().get_stats().messages, messages);

    auto *message_with_extra_memory = SHMM::get().acquire_shared_message<job_workers::JobSharedMessage>();
    ASSERT_ACQUIRED(SHMM::get().get_stats().messages, messages);
    check_new_message(message_with_extra_memory);
    // 256kb
    ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(200 * 1024));
    ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 200 * 1024));
    ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[0], _256kb);
    ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(200 * 1024));
    // 512kb
    ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(400 * 1024));
    ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 400 * 1024));
    ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[1], _512kb);
    ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(400 * 1024));
    // 1mb
    ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(800 * 1024));
    ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 800 * 1024));
    ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[2], _1mb);
    ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(800 * 1024));
    // 2mb
    if (vk::any_of_equal(logname_id, 1, 2, 3, 4)) {
      ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(1500 * 1024));
      ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 1500 * 1024));
      ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[3], _2mb);
      ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(1500 * 1024));
    }
    // 4mb
    if (vk::any_of_equal(logname_id, 1, 5)) {
      ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(3 * 1024 * 1024));
      ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 3 * 1024 * 1024));
      ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[4], _4mb);
      ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(3 * 1024 * 1024));
    }
    SHMM::get().release_shared_message(message_with_extra_memory);
    ASSERT_RELEASED(SHMM::get().get_stats().messages, messages);
    ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[0], _256kb);
    ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[1], _512kb);
    ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[2], _1mb);
    if (vk::any_of_equal(logname_id, 1, 2, 3, 4)) {
      ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[3], _2mb);
    }
    if (vk::any_of_equal(logname_id, 1, 5)) {
      ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[4], _4mb);
    }
  }
}

#define ASSERT_BUFFER(stat, expected_count, expected_acquired)                                                                                                 \
  ASSERT_EQ((stat).count, (expected_count));                                                                                                                   \
  ASSERT_EQ((stat).acquire_fails, 0);                                                                                                                          \
  ASSERT_EQ((stat).acquired, (expected_acquired));                                                                                                             \
  ASSERT_EQ((stat).released, (expected_acquired));

TEST(shared_memory_manager_test, test_manager) {
  SHMM::get().set_memory_limit(256 * 1024 * 1024);
  SHMM::get().set_shared_memory_distribution_weights({2, 2, 2, 2, 1, 1, 1, 1, 1, 1});
  vk::singleton<WorkersControl>::get().set_total_workers_count(7);
  SHMM::get().init();

  const auto &stats = SHMM::get().get_stats();
  ASSERT_EQ(stats.memory_limit, 256 * 1024 * 1024);
  ASSERT_LT(stats.unused_memory, 1 << JOB_SHARED_MESSAGE_SIZE_EXP);

  std::array<pid_t, 5> children{};
  for (int i = 0; i != children.size(); ++i) {
    auto child_pid = fork();
    if (!child_pid) {
      prctl(PR_SET_PDEATHSIG, SIGKILL);
      logname_id = i + 1;
      worker_function();
      _exit(testing::Test::HasFailure());
    }
    children[i] = child_pid;
  }

  for (auto child_pid : children) {
    int status = 0;
    ASSERT_GE(waitpid(child_pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
  }

  ASSERT_BUFFER(stats.messages, 293, 150000);
  // 256kb
  ASSERT_BUFFER(stats.extra_memory[0], 147, 50000);
  // 512kb
  ASSERT_BUFFER(stats.extra_memory[1], 73, 50000);
  // 1mb
  ASSERT_BUFFER(stats.extra_memory[2], 36, 50000);
  // 2mb
  ASSERT_BUFFER(stats.extra_memory[3], 9, 40000);
  // 4mb
  ASSERT_BUFFER(stats.extra_memory[4], 5, 20000);
  // 8mb
  ASSERT_BUFFER(stats.extra_memory[5], 3, 0);
  // 16mb
  ASSERT_BUFFER(stats.extra_memory[6], 1, 0);
  // 32mb
  ASSERT_BUFFER(stats.extra_memory[7], 1, 0);
  // 64mb
  ASSERT_BUFFER(stats.extra_memory[8], 0, 0);
}

TEST(shared_memory_manager_test, test_size_by_bucket_id) {
  ASSERT_EQ(get_extra_shared_memory_buffer_size(0), 256 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(1), 512 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(2), 1 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(3), 2 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(4), 4 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(5), 8 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(6), 16 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(7), 32 * 1024 * 1024);
  ASSERT_EQ(get_extra_shared_memory_buffer_size(8), 64 * 1024 * 1024);
}

TEST(shared_memory_manager_test, test_bucket_id_by_size) {
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(256 * 1024), 0);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(512 * 1024), 1);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(1 * 1024 * 1024), 2);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(2 * 1024 * 1024), 3);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(4 * 1024 * 1024), 4);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(8 * 1024 * 1024), 5);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(16 * 1024 * 1024), 6);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(32 * 1024 * 1024), 7);
  ASSERT_EQ(get_extra_shared_memory_buffers_group_idx(64 * 1024 * 1024), 8);
}

void check_distribution(size_t buffer_size, const std::array<size_t, 10> &actual, const std::array<size_t, 10> &expected) {
  size_t total_used_mem = 0;
  for (int i = 0; i < 10; ++i) {
    size_t cnt = actual[i];
    ASSERT_EQ(cnt, expected[i]);
    total_used_mem += cnt * (1 << (JOB_SHARED_MESSAGE_SIZE_EXP + i));
  }
  ASSERT_LE(total_used_mem, buffer_size);
}

TEST(shared_memory_manager_test, test_memory_distribution_not_enough_memory) {
  const size_t buffer_size = JOB_SHARED_MESSAGE_BYTES - 1;
  SHMM::get().set_shared_memory_distribution_weights({2, 2, 2, 2, 1, 1, 1, 1, 1, 1});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), buffer_size);
  check_distribution(buffer_size, buffers_distribution, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

TEST(shared_memory_manager_test, test_memory_distribution_left_some_memory) {
  constexpr size_t buffer_size = 5 * 1024 * 1024 + (JOB_SHARED_MESSAGE_BYTES - 1);
  auto buffer = std::make_unique<uint8_t[]>(buffer_size);
  SHMM::get().set_shared_memory_distribution_weights({2, 2, 2, 2, 1, 1, 1, 1, 1, 1});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), (JOB_SHARED_MESSAGE_BYTES - 1));
  check_distribution(buffer_size, buffers_distribution, {6, 3, 1, 1, 1, 0, 0, 0, 0, 0});
}

TEST(shared_memory_manager_test, test_memory_distribution_more_than_required) {
  constexpr size_t buffer_size = 701 * 1024 * 1024 + (JOB_SHARED_MESSAGE_BYTES - 1);
  SHMM::get().set_shared_memory_distribution_weights({2, 2, 2, 2, 1, 1, 1, 1, 1, 1});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), (JOB_SHARED_MESSAGE_BYTES - 1));
  check_distribution(buffer_size, buffers_distribution, {802, 401, 201, 100, 26, 12, 7, 3, 1, 1});
}

TEST(shared_memory_manager_test, test_no_extra_buffers) {
  constexpr size_t buffer_size = 100 * JOB_SHARED_MESSAGE_BYTES;
  SHMM::get().set_shared_memory_distribution_weights({1, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), 0);
  check_distribution(buffer_size, buffers_distribution, {100, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

TEST(shared_memory_manager_test, test_equal_distribution) {
  constexpr size_t buffer_size = 512l * 10 * 1024 * 1024;
  SHMM::get().set_shared_memory_distribution_weights({1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), 0);
  check_distribution(buffer_size, buffers_distribution, {4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8});
}

TEST(shared_memory_manager_test, test_float_distribution) {
  constexpr size_t buffer_size = 300l * 7 * 1024 * 1024; // 300*7 = 2100MB
  SHMM::get().set_shared_memory_distribution_weights({0.5, 0.5, 0.5, 0.5, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), 0);
  check_distribution(buffer_size, buffers_distribution, {2400, 1200, 600, 300, 76, 37, 19, 10, 5, 2});
}

TEST(shared_memory_manager_test, test_memory_distribution) {
  constexpr size_t buffer_size = 300l * 7 * 1024 * 1024; // 300*7 = 2100MB
  SHMM::get().set_shared_memory_distribution_weights({2, 2, 2, 2, 1, 1, 1, 1, 1, 1});
  std::array<size_t, 1 + JOB_EXTRA_MEMORY_BUFFER_BUCKETS> buffers_distribution;
  ASSERT_EQ(SHMM::get().calc_shared_memory_buffers_distribution(buffer_size, buffers_distribution), 0);
  check_distribution(buffer_size, buffers_distribution, {2400, 1200, 600, 300, 76, 37, 19, 10, 5, 2});
}
