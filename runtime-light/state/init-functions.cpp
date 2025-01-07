// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/init-functions.h"

#include <cinttypes>
#include <cstdint>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/http/init-functions.h"
#include "runtime-light/server/init-functions.h"
#include "runtime-light/server/job-worker/job-worker-server-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"

namespace {

void process_k2_invoke_http(tl::TLBuffer &tlb) noexcept {
  tl::K2InvokeHttp invoke_http{};
  if (!invoke_http.fetch(tlb)) {
    php_error("erroneous http request");
  }
  init_server(ServerQuery{std::move(invoke_http)});
}

void process_k2_invoke_job_worker(tl::TLBuffer &tlb) noexcept {
  tl::K2InvokeJobWorker invoke_jw{};
  if (!invoke_jw.fetch(tlb)) {
    php_error("erroneous job worker request");
  }
  php_assert(invoke_jw.image_id == k2::describe()->build_timestamp); // ensure that we got the request from ourselves
  init_server(ServerQuery{invoke_jw});
}

} // namespace

task_t<uint64_t> init_kphp_cli_component() noexcept {
  { // TODO superglobals init
    auto &superglobals{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals()};
    using namespace PhpServerSuperGlobalIndices;
    superglobals.v$argc = static_cast<int64_t>(0);
    superglobals.v$argv = array<mixed>{};
    superglobals.v$_SERVER.set_value(string{ARGC.data(), ARGC.size()}, superglobals.v$argc);
    superglobals.v$_SERVER.set_value(string{ARGV.data(), ARGV.size()}, superglobals.v$argv);
    superglobals.v$_SERVER.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, string{});
    superglobals.v$_SERVER.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, string{});
  }
  co_return co_await wait_for_incoming_stream_t{};
}

task_t<void> finalize_kphp_cli_component(const string_buffer &output) noexcept {
  auto &instance_st{InstanceState::get()};
  if ((co_await write_all_to_stream(instance_st.standard_stream(), output.buffer(), output.size())) != output.size()) [[unlikely]] {
    instance_st.poll_status = k2::PollStatus::PollFinishedError;
    php_warning("can't write component result to stream %" PRIu64, instance_st.standard_stream());
  }
}

task_t<uint64_t> init_kphp_server_component() noexcept {
  auto stream_d{co_await wait_for_incoming_stream_t{}};
  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  php_assert(size >= sizeof(uint32_t)); // check that we can fetch at least magic
  tl::TLBuffer tlb{};
  tlb.store_bytes({buffer, static_cast<size_t>(size)});
  k2::free(buffer);

  switch (const auto magic{*tlb.lookup_trivial<uint32_t>()}) { // lookup magic
    case tl::K2_INVOKE_HTTP_MAGIC: {
      process_k2_invoke_http(tlb);
      break;
    }
    case tl::K2_INVOKE_JOB_WORKER_MAGIC: {
      process_k2_invoke_job_worker(tlb);
      // release standard stream in case of a no reply job worker since we don't need that stream anymore
      if (JobWorkerServerInstanceState::get().kind == JobWorkerServerInstanceState::Kind::NoReply) {
        InstanceState::get().release_stream(stream_d);
        stream_d = k2::INVALID_PLATFORM_DESCRIPTOR;
      }
      break;
    }
    default: {
      php_error("unexpected magic: 0x%x", magic);
    }
  }
  co_return stream_d;
}

task_t<void> finalize_kphp_server_component(const string_buffer &output) noexcept {
  if (JobWorkerServerInstanceState::get().kind == JobWorkerServerInstanceState::Kind::Invalid) {
    co_await kphp::http::finalize_server(output);
  }
}
