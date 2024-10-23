// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime-common/runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime/job-workers/job-interface.h"

namespace job_workers {

constexpr size_t JOB_SHARED_MESSAGE_SIZE_EXP = 17;
// the size of the job shared message (without extra memory)
constexpr size_t JOB_SHARED_MESSAGE_BYTES = 1 << JOB_SHARED_MESSAGE_SIZE_EXP; // 128KB
// the number of buckets for extra shared memory,
//      0 => 256KB, 1 => 512KB, 2 => 1MB, 3 => 2MB, 4 => 4MB, 5 => 8MB, 6 => 16MB, 7 => 32MB, 8 => 64MB
constexpr size_t JOB_EXTRA_MEMORY_BUFFER_BUCKETS = 9;
// the default multiplier for getting shared memory limit for job workers messaging:
//    the default value for shared memory = the processes number * JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER
constexpr size_t JOB_DEFAULT_MEMORY_LIMIT_PROCESS_MULTIPLIER = 8 * 1024 * 1024; // 8MB for 1 process

inline int get_extra_shared_memory_buffers_group_idx(size_t size) noexcept {
  return __builtin_ctzll(size) - (JOB_SHARED_MESSAGE_SIZE_EXP + 1);
}

inline size_t get_extra_shared_memory_buffer_size(size_t group_idx) noexcept {
  return 1 << (JOB_SHARED_MESSAGE_SIZE_EXP + 1 + group_idx);
}

struct JobSharedMemoryPiece;

struct alignas(8) JobMetadata : vk::not_copyable {
  memory_resource::unsynchronized_pool_resource resource;

  class_instance<SendingInstanceBase> instance;
  std::atomic<uint32_t> owners_counter{1};

  virtual JobSharedMemoryPiece *get_common_job() const noexcept {
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
  JobSharedMemoryPiece *common_job{nullptr};
  bool no_reply{false};

  double job_deadline_time() const noexcept {
    return job_start_time + job_timeout;
  }

  void bind_common_job(JobSharedMemoryPiece *job_shared_memory_piece) noexcept;
  void unbind_common_job() noexcept;

  JobSharedMemoryPiece *get_common_job() const noexcept override {
    return common_job;
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

inline void JobSharedMessageMetadata::bind_common_job(JobSharedMemoryPiece *job_shared_memory_piece) noexcept {
  assert(job_shared_memory_piece);
  common_job = job_shared_memory_piece;
  ++common_job->owners_counter;
}

inline void JobSharedMessageMetadata::unbind_common_job() noexcept {
  assert(common_job);
  --common_job->owners_counter;
  assert(common_job->owners_counter != 0);
  common_job = nullptr;
}

} // namespace job_workers
