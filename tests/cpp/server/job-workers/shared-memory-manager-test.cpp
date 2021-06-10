// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>

#include "common/macos-ports.h"

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

#define ASSERT_ACQUIRED(stat, before) {     \
  const size_t new_stat = (stat).acquired;  \
  ASSERT_GT(new_stat, before.acquired);     \
  before.acquired = new_stat;               \
}

#define ASSERT_RELEASED(stat, before) {     \
  const size_t new_stat = (stat).released;  \
  ASSERT_GT(new_stat, before.released);     \
  before.released = new_stat;               \
}

void worker_function() {
  AcquiredReleased messages;
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
    // 1mb x 8
    ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(800 * 1024));
    ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 800 * 1024));
    ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[0], _1mb);
    ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(800 * 1024));
    // 2mb x 4
    if (vk::any_of_equal(logname_id, 1, 2, 3, 4)) {
      ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(1500 * 1024));
      ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 1500 * 1024));
      ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[1], _2mb);
      ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(1500 * 1024));
    }
    // 4mb x 2
    if (vk::any_of_equal(logname_id, 1, 5)) {
      ASSERT_FALSE(message_with_extra_memory->resource.is_enough_memory_for(3 * 1024 * 1024));
      ASSERT_TRUE(SHMM::get().request_extra_memory_for_resource(message_with_extra_memory->resource, 3 * 1024 * 1024));
      ASSERT_ACQUIRED(SHMM::get().get_stats().extra_memory[2], _4mb);
      ASSERT_TRUE(message_with_extra_memory->resource.is_enough_memory_for(3 * 1024 * 1024));
    }
    SHMM::get().release_shared_message(message_with_extra_memory);
    ASSERT_RELEASED(SHMM::get().get_stats().messages, messages);
    ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[0], _1mb);
    if (vk::any_of_equal(logname_id, 1, 2, 3, 4)) {
      ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[1], _2mb);
    }
    if (vk::any_of_equal(logname_id, 1, 5)) {
      ASSERT_RELEASED(SHMM::get().get_stats().extra_memory[2], _4mb);
    }
  }
}

#define ASSERT_BUFFER(stat, expected_count, expected_acquired)  \
  ASSERT_EQ((stat).count, (expected_count));                    \
  ASSERT_EQ((stat).acquire_fails, 0);                           \
  ASSERT_EQ((stat).acquired, (expected_acquired));              \
  ASSERT_EQ((stat).released, (expected_acquired));

TEST(shared_memory_manager_test, test_manager) {
  SHMM::get().set_memory_limit(256 * 1024 * 1024);
  vk::singleton<WorkersControl>::get().set_total_workers_count(7);
  SHMM::get().init();

  const auto &stats = SHMM::get().get_stats();
  ASSERT_EQ(stats.memory_limit, 256 * 1024 * 1024);
  ASSERT_GT(stats.unused_memory, 1000000); // 1mb == 1 048 576
  ASSERT_LT(stats.unused_memory, 1048576);

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

  ASSERT_BUFFER(stats.messages, 14, 150000);
  // 1mb
  ASSERT_BUFFER(stats.extra_memory[0], 8, 50000);
  // 2mb
  ASSERT_BUFFER(stats.extra_memory[1], 4, 40000);
  // 4mb
  ASSERT_BUFFER(stats.extra_memory[2], 2, 20000);
  // 8mb
  ASSERT_BUFFER(stats.extra_memory[3], 2, 0);
  // 16mb
  ASSERT_BUFFER(stats.extra_memory[4], 1, 0);
  // 32mb
  ASSERT_BUFFER(stats.extra_memory[5], 2, 0);
  // 64mb
  ASSERT_BUFFER(stats.extra_memory[6], 2, 0);
}
