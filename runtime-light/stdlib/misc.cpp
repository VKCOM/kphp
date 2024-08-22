// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/misc.h"

#include <cstdint>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/header.h"
#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"

namespace {

int32_t ob_merge_buffers() {
  Response &response = get_component_context()->response;
  php_assert(response.current_buffer >= 0);
  int32_t ob_first_not_empty = 0;
  while (ob_first_not_empty < response.current_buffer && response.output_buffers[ob_first_not_empty].size() == 0) {
    ob_first_not_empty++;
  }
  for (auto i = ob_first_not_empty + 1; i <= response.current_buffer; i++) {
    response.output_buffers[ob_first_not_empty].append(response.output_buffers[i].c_str(), response.output_buffers[i].size());
  }
  return ob_first_not_empty;
}

} // namespace

task_t<uint64_t> wait_and_process_incoming_stream(QueryType query_type) {
  const auto incoming_stream_d{co_await wait_for_incoming_stream_t{}};
  switch (query_type) {
    case QueryType::HTTP: {
      const auto [buffer, size] = co_await read_all_from_stream(incoming_stream_d);
      init_http_superglobals(buffer, size);
      get_platform_context()->allocator.free(buffer);
      break;
    }
    case QueryType::COMPONENT: { // processing takes place in a component
      break;
    }
  }
  co_return incoming_stream_d;
}

task_t<void> finish(int64_t exit_code, bool from_exit) { // TODO: use exit code
  (void)from_exit;
  (void)exit_code;
  auto &component_ctx{*get_component_context()};
  const auto standard_stream{component_ctx.standard_stream()};
  if (standard_stream == INVALID_PLATFORM_DESCRIPTOR) {
    component_ctx.poll_status = PollStatus::PollFinishedError;
    co_return;
  }

  const auto ob_total_buffer = ob_merge_buffers();
  Response &response = component_ctx.response;
  auto &buffer = response.output_buffers[ob_total_buffer];
  if ((co_await write_all_to_stream(standard_stream, buffer.c_str(), buffer.size())) != buffer.size()) {
    php_warning("can't write component result to stream %" PRIu64, standard_stream);
  }
  component_ctx.release_all_streams();
  component_ctx.poll_status = PollStatus::PollFinishedOk;
}

void f$check_shutdown() {
  const auto &platform_ctx{*get_platform_context()};
  if (static_cast<bool>(get_platform_context()->please_graceful_shutdown.load())) {
    php_notice("script was graceful shutdown");
    platform_ctx.abort();
  }
}

task_t<void> f$exit(const mixed &v) {
  if (v.is_string()) {
    Response &response = get_component_context()->response;
    response.output_buffers[response.current_buffer] << v;
    co_await finish(0, true);
  } else {
    co_await finish(v.to_int(), true);
  }
  critical_error_handler();
}

void f$die([[maybe_unused]] const mixed &v) {
  get_component_context()->poll_status = PollStatus::PollFinishedOk;
  critical_error_handler();
}
