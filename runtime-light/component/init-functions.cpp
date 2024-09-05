// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/init-functions.h"

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

void process_k2_invoke_job_worker(tl::TLBuffer &tlb) noexcept {
  tl::K2InvokeJobWorker invoke_jw{};
  if (!invoke_jw.fetch(tlb)) {
    php_error("erroneous job worker request");
  }
  php_assert(invoke_jw.image_id == vk_k2_describe()->build_timestamp); // check that we got the request from the same image
  auto &jw_server_ctx{JobWorkerServerComponentContext::get()};
  jw_server_ctx.job_id = invoke_jw.job_id;
  jw_server_ctx.body = std::move(invoke_jw.body);
  get_component_context()->php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string{"JOB_ID"}, invoke_jw.job_id);
}

task_t<uint64_t> init_kphp_server_component() noexcept {
  const auto stream_d{co_await wait_for_incoming_stream_t{}};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  php_assert(size >= sizeof(uint32_t)); // check that we can fetch at least magic
  tl::TLBuffer tlb{};
  tlb.store_bytes(buffer, static_cast<size_t>(size));
  get_platform_context()->allocator.free(buffer);

  switch (const auto magic{*reinterpret_cast<const uint32_t *>(tlb.data())}) { // lookup magic
    case tl::K2_INVOKE_HTTP_MAGIC: {
      process_k2_invoke_http(tlb);
      break;
    }
    case tl::K2_INVOKE_JOB_WORKER_MAGIC: {
      process_k2_invoke_job_worker(tlb);
      break;
    }
    default: {
      php_error("unexpected magic: 0x%x", magic);
    }
  }
  co_return stream_d;
}
