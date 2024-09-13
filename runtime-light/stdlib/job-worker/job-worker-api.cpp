// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/job-worker/job-worker-api.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/header.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/fork/fork-context.h"
#include "runtime-light/stdlib/job-worker/job-worker-context.h"
#include "runtime-light/stdlib/job-worker/job-worker.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/context.h"

namespace {

constexpr const char *JOB_WORKER_COMPONENT_NAME = "_self";

constexpr double MIN_TIMEOUT_S = 0.05;
constexpr double MAX_TIMEOUT_S = 86400.0;

task_t<int64_t> kphp_job_worker_start_impl(string request, double timeout, bool ignore_answer) noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("can't start job worker: job workers are disabled");
    co_return INVALID_FORK_ID;
  }
  if (request.empty()) {
    php_warning("job worker request is empty");
    co_return INVALID_FORK_ID;
  }

  auto &jw_client_ctx{JobWorkerClientComponentContext::get()};
  // normalize timeout
  const auto timeout_ns{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(std::clamp(timeout, MIN_TIMEOUT_S, MAX_TIMEOUT_S)))};
  // prepare JW component request
  tl::TLBuffer tlb{};
  const tl::K2InvokeJobWorker invoke_jw{.image_id = vk_k2_describe()->build_timestamp,
                                        .job_id = jw_client_ctx.current_job_id++,
                                        .ignore_answer = ignore_answer,
                                        .timeout_ns = static_cast<uint64_t>(timeout_ns.count()),
                                        .body = std::move(request)};
  invoke_jw.store(tlb);

  // send JW request
  auto comp_query{co_await f$component_client_send_request(string{JOB_WORKER_COMPONENT_NAME}, string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (comp_query.is_null()) {
    php_warning("couldn't start job worker");
    co_return INVALID_FORK_ID;
  }
  // create fork to wait for job worker response. we need to do it even if 'ignore_answer' is 'true' to make sure
  // that the stream will not be closed too early. otherwise, platform may even not send job worker request
  auto waiter_task{[](auto comp_query, std::chrono::nanoseconds timeout) noexcept -> task_t<string> {
    auto fetch_task{f$component_client_fetch_response(std::move(comp_query))};
    const string response{(co_await wait_with_timeout_t{task_t<string>::awaiter_t{std::addressof(fetch_task)}, timeout}).value_or(string{})};

    tl::TLBuffer tlb{};
    tlb.store_bytes(response.c_str(), static_cast<size_t>(response.size()));
    tl::K2JobWorkerResponse jw_response{};
    if (!jw_response.fetch(tlb)) {
      co_return string{};
    }
    co_return std::move(jw_response.body);
  }(std::move(comp_query), timeout_ns)};
  // start waiter fork and return its ID
  co_return(co_await start_fork_t{static_cast<task_t<void>>(std::move(waiter_task)), start_fork_t::execution::self});
}

} // namespace

// ================================================================================================

task_t<Optional<int64_t>> f$kphp_job_worker_start(string request, double timeout) noexcept {
  const auto fork_id{co_await kphp_job_worker_start_impl(std::move(request), timeout, false)};
  co_return fork_id != INVALID_FORK_ID ? fork_id : false;
}

task_t<bool> f$kphp_job_worker_start_no_reply(string request, double timeout) noexcept {
  const auto fork_id{co_await kphp_job_worker_start_impl(std::move(request), timeout, true)};
  co_return fork_id != INVALID_FORK_ID;
}

task_t<array<Optional<int64_t>>> f$kphp_job_worker_start_multi(array<string> requests, double timeout) noexcept {
  array<Optional<int64_t>> fork_ids{requests.size()};
  for (const auto &it : requests) {
    const auto fork_id{co_await kphp_job_worker_start_impl(it.get_value(), timeout, false)};
    fork_ids.set_value(it.get_key(), fork_id != INVALID_FORK_ID ? fork_id : false);
  }
  co_return fork_ids;
}

// ================================================================================================

task_t<string> f$kphp_job_worker_fetch_request() noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    php_warning("couldn't fetch job worker request: job workers are disabled");
    co_return string{};
  }

  auto &jw_server_ctx{JobWorkerServerComponentContext::get()};
  if (jw_server_ctx.job_id == JOB_WORKER_INVALID_JOB_ID || jw_server_ctx.body.empty()) {
    php_warning("couldn't fetch job worker request");
    co_return string{};
  }
  co_return std::exchange(jw_server_ctx.body, string{});
}

task_t<int64_t> f$kphp_job_worker_store_response(string response) noexcept {
  auto &component_ctx{*get_component_context()};
  auto &jw_server_ctx{JobWorkerServerComponentContext::get()};
  if (!f$is_kphp_job_workers_enabled()) { // workers are enabled
    php_warning("couldn't store job worker response: job workers are disabled");
    co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  } else if (jw_server_ctx.kind != JobWorkerServerComponentContext::Kind::Regular) { // we're in regular worker
    php_warning("couldn't store job worker response: we are either in no reply job worker or not in a job worker at all");
    co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  } else if (jw_server_ctx.state == JobWorkerServerComponentContext::State::Replied) { // it's the first attempt to reply
    php_warning("couldn't store job worker response: multiple stores are forbidden");
    co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  } else if (component_ctx.standard_stream() == INVALID_PLATFORM_DESCRIPTOR) { // we have a stream to write into
    php_warning("couldn't store job worker response: no standard stream");
    co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  } else if (response.empty()) { // we have a response to reply
    php_warning("couldn't store job worker response: it shouldn't be empty");
    co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  }

  tl::TLBuffer tlb{};
  tl::K2JobWorkerResponse jw_response{.job_id = jw_server_ctx.job_id, .body = std::move(response)};
  jw_response.store(tlb);
  if ((co_await write_all_to_stream(component_ctx.standard_stream(), tlb.data(), tlb.size())) != tlb.size()) {
    php_warning("couldn't store job worker response");
    co_return static_cast<int64_t>(JobWorkerError::store_response_cant_send_error);
  }
  jw_server_ctx.state = JobWorkerServerComponentContext::State::Replied;
  co_return 0;
}
