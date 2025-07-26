// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/job-worker/job-worker-api.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/server/job-worker/job-worker-server-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h"
#include "runtime-light/stdlib/job-worker/job-worker-client-state.h"
#include "runtime-light/stdlib/job-worker/job-worker.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/logs.h"

namespace {

constexpr const char* JOB_WORKER_COMPONENT_NAME = "_self";

constexpr double MIN_TIMEOUT_S = 0.05;
constexpr double MAX_TIMEOUT_S = 86400.0;

kphp::coro::task<int64_t> kphp_job_worker_start_impl(string request, double timeout, bool ignore_answer) noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    kphp::log::warning("can't start job worker: job workers are disabled");
    co_return kphp::forks::INVALID_ID;
  }
  if (request.empty()) {
    kphp::log::warning("job worker request is empty");
    co_return kphp::forks::INVALID_ID;
  }

  auto& jw_client_st{JobWorkerClientInstanceState::get()};
  // normalize timeout
  const auto timeout_ns{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(std::clamp(timeout, MIN_TIMEOUT_S, MAX_TIMEOUT_S)))};
  // prepare JW component request
  tl::TLBuffer tlb{};
  const tl::K2InvokeJobWorker invoke_jw{.image_id = {.value = static_cast<int64_t>(k2::describe()->build_timestamp)},
                                        .job_id = {.value = jw_client_st.current_job_id++},
                                        .ignore_answer = ignore_answer,
                                        .timeout_ns = {.value = timeout_ns.count()},
                                        .body = {.value = {request.c_str(), request.size()}}};
  invoke_jw.store(tlb);

  // send JW request
  auto comp_query{co_await f$component_client_send_request(string{JOB_WORKER_COMPONENT_NAME}, string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  if (comp_query.is_null()) {
    kphp::log::warning("couldn't start job worker");
    co_return kphp::forks::INVALID_ID;
  }
  // create fork to wait for job worker response. we need to do it even if 'ignore_answer' is 'true' to make sure
  // that the stream will not be closed too early. otherwise, platform may even not send job worker request
  auto waiter_task{[](auto comp_query, std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<string> {
    auto expected{co_await kphp::coro::io_scheduler::get().schedule(f$component_client_fetch_response(std::move(comp_query)), timeout)};
    string response{expected ? std::move(*expected) : string{}};

    tl::TLBuffer tlb{};
    tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});
    tl::K2JobWorkerResponse jw_response{};
    if (!jw_response.fetch(tlb)) {
      co_return string{};
    }
    co_return string{jw_response.body.value.data(), static_cast<string::size_type>(jw_response.body.value.size())};
  }(std::move(comp_query), timeout_ns)};
  // start waiter fork and return its ID
  co_return kphp::forks::start(std::move(waiter_task));
}

} // namespace

// ================================================================================================

kphp::coro::task<Optional<int64_t>> f$job_worker_send_request(string request, double timeout) noexcept {
  const auto fork_id{co_await kphp_job_worker_start_impl(std::move(request), timeout, false)};
  co_return fork_id != kphp::forks::INVALID_ID ? fork_id : false;
}

kphp::coro::task<bool> f$job_worker_send_noreply_request(string request, double timeout) noexcept {
  const auto fork_id{co_await kphp_job_worker_start_impl(std::move(request), timeout, true)};
  co_return fork_id != kphp::forks::INVALID_ID;
}

kphp::coro::task<array<Optional<int64_t>>> f$job_worker_send_multi_request(array<string> requests, double timeout) noexcept {
  array<Optional<int64_t>> fork_ids{requests.size()};
  for (const auto& it : requests) {
    const auto fork_id{co_await kphp_job_worker_start_impl(it.get_value(), timeout, false)};
    fork_ids.set_value(it.get_key(), fork_id != kphp::forks::INVALID_ID ? fork_id : false);
  }
  co_return fork_ids;
}

// ================================================================================================

kphp::coro::task<string> f$job_worker_fetch_request() noexcept {
  if (!f$is_kphp_job_workers_enabled()) {
    kphp::log::warning("couldn't fetch job worker request: job workers are disabled");
    co_return string{};
  }

  auto& jw_server_st{JobWorkerServerInstanceState::get()};
  if (jw_server_st.job_id == JOB_WORKER_INVALID_JOB_ID || jw_server_st.body.empty()) {
    kphp::log::warning("couldn't fetch job worker request");
    co_return string{};
  }
  co_return std::exchange(jw_server_st.body, string{});
}

kphp::coro::task<int64_t> f$job_worker_store_response(string response) noexcept {
  // auto& instance_st{InstanceState::get()};
  // auto& jw_server_st{JobWorkerServerInstanceState::get()};
  // if (!f$is_kphp_job_workers_enabled()) { // workers are enabled
  //   kphp::log::warning("couldn't store job worker response: job workers are disabled");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  // } else if (jw_server_st.kind != JobWorkerServerInstanceState::Kind::Regular) { // we're in regular worker
  //   kphp::log::warning("couldn't store job worker response: we are either in no reply job worker or not in a job worker at all");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  // } else if (jw_server_st.state == JobWorkerServerInstanceState::State::Replied) { // it's the first attempt to reply
  //   kphp::log::warning("couldn't store job worker response: multiple stores are forbidden");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  // } else if (instance_st.standard_stream() == k2::INVALID_PLATFORM_DESCRIPTOR) { // we have a stream to write into
  //   kphp::log::warning("couldn't store job worker response: no standard stream");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  // } else if (response.empty()) { // we have a response to reply
  //   kphp::log::warning("couldn't store job worker response: it shouldn't be empty");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
  // }
  //
  // tl::TLBuffer tlb{};
  // tl::K2JobWorkerResponse{.job_id = {.value = jw_server_st.job_id}, .body = {.value = {response.c_str(), response.size()}}}.store(tlb);
  // if ((co_await write_all_to_stream(instance_st.standard_stream(), tlb.data(), tlb.size())) != tlb.size()) {
  //   kphp::log::warning("couldn't store job worker response");
  //   co_return static_cast<int64_t>(JobWorkerError::store_response_cant_send_error);
  // }
  // jw_server_st.state = JobWorkerServerInstanceState::State::Replied;
  co_return 0;
}
