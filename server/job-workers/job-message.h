// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/job-workers/job-interface.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/kphp_core.h"

namespace job_workers {

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

struct JobSharedMemoryPiece;

struct alignas(8) JobMetadata : vk::not_copyable {
  memory_resource::unsynchronized_pool_resource resource;

  class_instance<SendingInstanceBase> instance;
  std::atomic<uint32_t> owners_counter{1};

  virtual JobSharedMemoryPiece *get_parent_job() const noexcept {
    return nullptr;
  }

  JobMetadata() = default;
  ~JobMetadata() = default;
};


struct alignas(8) JobSharedMessageMetadata : JobMetadata {
public:
  int32_t job_id{0};
  int32_t job_result_fd_idx{-1};
  double job_start_time{-1.0};
  double job_timeout{-1.0};
  JobSharedMemoryPiece *parent_job{nullptr};

  double job_deadline_time() const noexcept {
    return job_start_time + job_timeout;
  }

  void bind_parent_job(JobSharedMemoryPiece *parent) noexcept;
  void unbind_parent_job() noexcept;

  JobSharedMemoryPiece *get_parent_job() const noexcept override {
    return parent_job;
  }

protected:
  JobSharedMessageMetadata() = default;
  ~JobSharedMessageMetadata() = default;
};

template <typename Metadata>
struct alignas(8) GenericJobMessage : Metadata {
private:
  static constexpr size_t MEMORY_POOL_BUFFER_SIZE = JOB_SHARED_MESSAGE_BYTES - sizeof(Metadata);

public:
  GenericJobMessage() noexcept {
    this->resource.init(memory_pool_buffer_, MEMORY_POOL_BUFFER_SIZE);
  }

private:
  alignas(8) uint8_t memory_pool_buffer_[MEMORY_POOL_BUFFER_SIZE];
};

struct alignas(8) JobSharedMessage : GenericJobMessage<JobSharedMessageMetadata> {
// It's not a typedef because we need to forward declare this struct in several places
};

struct alignas(8) JobSharedMemoryPiece : GenericJobMessage<JobMetadata> {
// It's not a typedef because we need to forward declare this struct in several places
};

static_assert(sizeof(JobSharedMemoryPiece) == JOB_SHARED_MESSAGE_BYTES, "check yourself");
static_assert(sizeof(JobSharedMessage) == JOB_SHARED_MESSAGE_BYTES, "check yourself");

inline void JobSharedMessageMetadata::bind_parent_job(JobSharedMemoryPiece *parent) noexcept {
  assert(parent);
  parent_job = parent;
  ++parent_job->owners_counter;
}

inline void JobSharedMessageMetadata::unbind_parent_job() noexcept {
  assert(parent_job);
  --parent_job->owners_counter;
  assert(parent_job->owners_counter != 0);
  parent_job = nullptr;
}

} // namespace job_workers
