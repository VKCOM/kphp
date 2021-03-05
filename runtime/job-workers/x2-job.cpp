// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/wrappers/string_view.h"
#include "common/algorithms/hashes.h"

#include "runtime/job-workers/client-functions.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/job-workers/processing-jobs.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/net_events.h"
#include "runtime/refcountable_php_classes.h"

#include "server/job-workers/shared-context.h"
#include "server/job-workers/job-worker-client.h"

#include "runtime/job-workers/x2-job.h"

namespace job_workers {

template<class Base>
struct JobX2Data final : refcountable_polymorphic_php_classes<Base> {
  const char *get_class() const final { return "JobX2Data"; }

  int64_t get_hash() const final { return vk::std_hash(vk::string_view(get_class())); }

  void accept(InstanceDeepCopyVisitor &visitor) final { visitor("data", data); }

  void accept(InstanceDeepDestroyVisitor &visitor) final { visitor("data", data); }

  size_t virtual_builtin_sizeof() const final { return sizeof(JobX2Data); }

  JobX2Data *virtual_builtin_clone() const final { return new JobX2Data{*this}; }

  array<int64_t> data;
};

using JobX2Request = JobX2Data<C$KphpJobWorkerRequest>;
using JobX2Reply = JobX2Data<C$KphpJobWorkerReply>;


SharedMemorySlice *process_x2(SharedMemorySlice *req) noexcept {
  auto x2_req = req->instance.cast_to<JobX2Request>();
  php_assert(!x2_req.is_null());
  // This code is called from not initialized environment,
  // so we use request shared memory without script allocator initialization,
  // but in real life, we should use default script allocator
  dl::set_current_script_allocator(req->resource, true);

  auto x2_reply = make_instance<JobX2Reply>();
  x2_reply.get()->data = std::move(x2_req.get()->data);
  for (auto &kv : x2_reply.get()->data) {
    kv.get_value() *= 2;
  }

  dl::restore_default_script_allocator(true);

  auto &memory_manager = vk::singleton<SharedMemoryManager>::get();
  SharedMemorySlice *reply = memory_manager.acquire_slice();
  php_assert(reply);
  reply->instance = copy_instance_into_other_memory(x2_reply, reply->resource, ExtraRefCnt::for_job_worker_communication, true);

  memory_manager.release_slice(req);
  return reply;
}

} // namespace job_workers

int64_t f$async_x2(const array<int64_t> &arr) noexcept {
  auto x2_req = make_instance<job_workers::JobX2Request>();
  x2_req.get()->data = arr;
  return f$kphp_job_worker_start(x2_req);
}

array<int64_t> f$await_x2(int64_t job_id) noexcept {
  auto x2_reply = f$kphp_job_worker_wait(job_id).cast_to<job_workers::JobX2Reply>();
  php_assert(!x2_reply.is_null());
  php_assert(x2_reply.get_reference_counter() == 1);
  php_assert(x2_reply->data.get_reference_counter() == 1);
  return std::move(x2_reply->data);
}
