// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/job-workers/job-interface.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/kphp_core.h"

namespace job_workers {

constexpr uint32_t JOB_SHARED_MESSAGE_FREE_CNT = std::numeric_limits<uint32_t>::max();
constexpr size_t JOB_SHARED_MESSAGE_SIZE = 1024 * 1024;

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

struct JobSharedMessage : JobSharedMessageMetadata {
private:
  static constexpr size_t MEMORY_POOL_BUFFER_SIZE = JOB_SHARED_MESSAGE_SIZE - sizeof(JobSharedMessageMetadata);

public:
  JobSharedMessage() noexcept {
    resource.init(memory_pool_buffer_, MEMORY_POOL_BUFFER_SIZE);
  }

private:
  alignas(8) char memory_pool_buffer_[MEMORY_POOL_BUFFER_SIZE];
};

static_assert(sizeof(JobSharedMessage) == JOB_SHARED_MESSAGE_SIZE, "check yourself");

} // namespace job_workers
