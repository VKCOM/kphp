//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/worker/worker_api.h"

#include <cinttypes>

#include "common/containers/final_action.h"
#include "runtime-light/component/component.h"
#include "runtime-light/stdlib/worker/worker.h"
#include "runtime-light/stdlib/worker/worker_context.h"
#include "runtime-light/streams/interface.h"
#include "runtime-light/utils/context.h"
#include "runtime-light/utils/php_assert.h"

namespace {

task_t<int64_t> kphp_job_worker_start_impl(const string &request, double timeout, bool ignore_answer) noexcept { // TODO: deal with timeout
  if (request.empty()) {
    php_warning("the request is empty");
    co_return JOB_WORKER_INVALID_JOB_ID;
  }

  auto comp_query{co_await f$component_client_send_query(string{"worker"}, request)};
  if (comp_query.is_null()) {
    php_warning("could not start job worker");
    co_return JOB_WORKER_INVALID_JOB_ID;
  }

  if (ignore_answer) { // TODO: should we wait for answer implicitly as in rpc_send_impl?
    co_return JOB_WORKER_IGNORED_JOB_ID;
  }
  auto &worker_client_ctx{get_component_context()->job_worker_client_context};
  const auto job_id{worker_client_ctx.current_job_id++};
  worker_client_ctx.pending_queries.emplace(job_id, std::move(comp_query));
  co_return job_id;
}

} // namespace

// === Client =====================================================================================

task_t<Optional<int64_t>> f$kphp_job_worker_start(string request, double timeout) noexcept {
  co_return co_await kphp_job_worker_start_impl(request, timeout, false);
}

task_t<bool> f$kphp_job_worker_start_no_reply(string request, double timeout) noexcept {
  co_return(co_await kphp_job_worker_start_impl(request, timeout, true) == JOB_WORKER_IGNORED_JOB_ID);
}

task_t<string> f$kphp_job_worker_fetch_response(int64_t job_id) noexcept {
  auto &worker_client_ctx{get_component_context()->job_worker_client_context};
  auto it_comp_query{worker_client_ctx.pending_queries.find(job_id)};
  vk::final_action finalizer{[&worker_client_ctx, it_comp_query]() { worker_client_ctx.pending_queries.erase(it_comp_query); }};
  if (it_comp_query == worker_client_ctx.pending_queries.end()) {
    php_warning("could not find job with id %" PRId64, job_id);
    co_return string{};
  }
  co_return co_await f$component_client_get_result(it_comp_query->second);
}

// === Server =====================================================================================

task_t<string> f$kphp_job_worker_fetch_request() noexcept {
  auto comp_stream{co_await f$component_accept_stream()};

  auto &worker_server_ctx{get_component_context()->job_worker_server_context};
  if (!worker_server_ctx.stream_to_client.is_null() || f$ComponentStream$$is_read_closed(comp_stream)) {
    php_warning(!worker_server_ctx.stream_to_client.is_null() ? "trying to fetch job worker request multiple times"
                                                              : "client's stream is closed for read operation");
    co_return string{};
  }

  worker_server_ctx.stream_to_client = std::move(comp_stream);
  co_return co_await f$component_stream_read_all(worker_server_ctx.stream_to_client);
}

task_t<int64_t> f$kphp_job_worker_store_response(string response) noexcept {
  auto &worker_server_ctx{get_component_context()->job_worker_server_context};
  // TODO: is_please_shutdown_write?
  if (response.empty() || worker_server_ctx.stream_to_client.is_null()) {
    if (response.empty()) {
      php_warning("the response is empty");
      co_return static_cast<int64_t>(JobWorkerError::store_response_incorrect_call_error);
    } else {
      php_warning("stream to client is null");
      co_return static_cast<int64_t>(JobWorkerError::store_response_cant_send_error);
    }
  }

  const auto written_bytes{co_await f$component_stream_write_exact(worker_server_ctx.stream_to_client, response)};
  if (written_bytes != response.size()) {
    php_warning("written_bytes %" PRId64 " bytes, but the response consists of %" PRIu64 " bytes", written_bytes, response.size());
    co_return static_cast<int64_t>(JobWorkerError::store_response_cant_send_error);
  }
  co_return 0;
}

// === Misc =======================================================================================
