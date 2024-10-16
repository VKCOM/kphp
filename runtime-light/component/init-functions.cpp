// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/init-functions.h"

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/stdlib/job-worker/job-worker-context.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/utils/context.h"

namespace {

void process_k2_invoke_job_worker(tl::TLBuffer &tlb) noexcept {
  tl::K2InvokeJobWorker invoke_jw{};
  if (!invoke_jw.fetch(tlb)) {
    php_error("erroneous job worker request");
  }
  php_assert(invoke_jw.image_id == vk_k2_describe()->build_timestamp); // ensure that we got the request from ourselves

  auto &jw_server_ctx{JobWorkerServerComponentContext::get()};
  jw_server_ctx.kind = invoke_jw.ignore_answer ? JobWorkerServerComponentContext::Kind::NoReply : JobWorkerServerComponentContext::Kind::Regular;
  jw_server_ctx.state = JobWorkerServerComponentContext::State::Working;
  jw_server_ctx.job_id = invoke_jw.job_id;
  jw_server_ctx.body = std::move(invoke_jw.body);
  get_component_context()->php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string{"JOB_ID"}, invoke_jw.job_id);
}

void process_k2_invoke_http([[maybe_unused]] tl::TLBuffer &tlb) noexcept {}

} // namespace

task_t<uint64_t> init_kphp_server_component() noexcept {
  auto stream_d{co_await wait_for_incoming_stream_t{}};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  php_assert(size >= sizeof(uint32_t)); // check that we can fetch at least magic
  tl::TLBuffer tlb{};
  tlb.store_bytes({buffer, static_cast<size_t>(size)});
  get_platform_context()->allocator.free(buffer);

  switch (const auto magic{*reinterpret_cast<const uint32_t *>(tlb.data())}) { // lookup magic
    case tl::K2_INVOKE_HTTP_MAGIC: {
      process_k2_invoke_http(tlb);
      break;
    }
    case tl::K2_INVOKE_JOB_WORKER_MAGIC: {
      process_k2_invoke_job_worker(tlb);
      // release standard stream in case of a no reply job worker since we don't need that stream anymore
      if (JobWorkerServerComponentContext::get().kind == JobWorkerServerComponentContext::Kind::NoReply) {
        get_component_context()->release_stream(stream_d);
        stream_d = INVALID_PLATFORM_DESCRIPTOR;
      }
      break;
    }
    default: {
      php_error("unexpected magic: 0x%x", magic);
    }
  }
  co_return stream_d;
}
