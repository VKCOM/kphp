// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/exit/exit-functions.h"

#include <cstdint>

#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/header.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"

namespace {

int32_t ob_merge_buffers() noexcept {
  Response &response{get_component_context()->response};
  php_assert(response.current_buffer >= 0);

  int32_t ob_first_not_empty{};
  while (ob_first_not_empty < response.current_buffer && response.output_buffers[ob_first_not_empty].size() == 0) {
    ++ob_first_not_empty;
  }
  for (auto i = ob_first_not_empty + 1; i <= response.current_buffer; i++) {
    response.output_buffers[ob_first_not_empty].append(response.output_buffers[i].c_str(), response.output_buffers[i].size());
  }
  return ob_first_not_empty;
}

} // namespace

task_t<void> shutdown_script() noexcept {
  auto &component_ctx{*get_component_context()};
  const auto standard_stream{component_ctx.standard_stream()};
  if (standard_stream == INVALID_PLATFORM_DESCRIPTOR) {
    component_ctx.poll_status = PollStatus::PollFinishedError;
    co_return;
  }

  const auto &buffer{component_ctx.response.output_buffers[ob_merge_buffers()]};
  if ((co_await write_all_to_stream(standard_stream, buffer.buffer(), buffer.size())) != buffer.size()) {
    php_warning("can't write component result to stream %" PRIu64, standard_stream);
  }
}

task_t<void> f$exit(const mixed &v) noexcept { // TODO: make it synchronous
  int64_t exit_code{};
  if (v.is_string()) {
    Response &response{get_component_context()->response};
    response.output_buffers[response.current_buffer] << v;
  } else if (v.is_int()) {
    int64_t v_code{v.to_int()};
    // valid PHP exit codes: [0..254]
    exit_code = v_code >= 0 && v_code <= 254 ? v_code : 1;
  } else {
    exit_code = 1;
  }
  co_await shutdown_script();
  auto &component_ctx{*get_component_context()};
  component_ctx.poll_status =
    component_ctx.poll_status != PollStatus::PollFinishedError && exit_code == 0 ? PollStatus::PollFinishedOk : PollStatus::PollFinishedError;
  component_ctx.release_all_streams();
  get_platform_context()->abort();
}
