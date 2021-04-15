// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/job-workers/job-interface.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/kphp_core.h"

namespace job_workers {

// the value of counter for the free messages
constexpr uint32_t JOB_SHARED_MESSAGE_FREE_CNT = std::numeric_limits<uint32_t>::max();
// this constant is used for calculating total available messages count:
//    messages count = the processes number * JOB_SHARED_MESSAGES_COUNT_PROCESS_MULTIPLIER
constexpr size_t JOB_SHARED_MESSAGES_COUNT_PROCESS_MULTIPLIER = 2;
// the size of the job shared message (without extra memory)
constexpr size_t JOB_SHARED_MESSAGE_BYTES = 512 * 1024; // 512KB
// the number of buckets for extra shared memory,
//    it is started from (2 * JOB_SHARED_MESSAGE_BYTES) Bytes and double for the next:
//      0 => 1MB, 1 => 2MB, 2 => 4MB, 3 => 8MB, 4 => 16MB, 5 => 32MB, 6 => 64MB
constexpr size_t JOB_EXTRA_MEMORY_BUFFER_BUCKETS = 7;
// the default multiplier for getting shared memory limit for job workers messaging:
//    the default value for shared memory = the processes number * JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER
constexpr size_t JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER = 8 * 1024 * 1024; // 8MB for 1 process

struct alignas(8) JobSharedMessageMetadata : vk::not_copyable {
public:
  memory_resource::unsynchronized_pool_resource resource;
  class_instance<SendingInstanceBase> instance;

  std::atomic<uint32_t> owners_counter{JOB_SHARED_MESSAGE_FREE_CNT};

  int32_t job_id{0};
  int32_t job_result_fd_idx{-1};
  double job_deadline_time{-1.0};

protected:
  JobSharedMessageMetadata() = default;
  ~JobSharedMessageMetadata() = default;
};

struct alignas(8) JobSharedMessage : JobSharedMessageMetadata {
private:
  static constexpr size_t MEMORY_POOL_BUFFER_SIZE = JOB_SHARED_MESSAGE_BYTES - sizeof(JobSharedMessageMetadata);

public:
  JobSharedMessage() noexcept {
    resource.init(memory_pool_buffer_, MEMORY_POOL_BUFFER_SIZE);
  }

private:
  alignas(8) uint8_t memory_pool_buffer_[MEMORY_POOL_BUFFER_SIZE];
};

static_assert(sizeof(JobSharedMessage) == JOB_SHARED_MESSAGE_BYTES, "check yourself");

} // namespace job_workers
