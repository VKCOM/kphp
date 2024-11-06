// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/exit/exit-functions.h"

#include <cstdint>

#include "runtime-light/component/component.h"
#include "runtime-light/header.h"
#include "runtime-light/utils/context.h"

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
  auto &component_ctx{*get_component_context()};
  co_await component_ctx.run_instance_epilogue();
  component_ctx.poll_status =
    component_ctx.poll_status != PollStatus::PollFinishedError && exit_code == 0 ? PollStatus::PollFinishedOk : PollStatus::PollFinishedError;
  component_ctx.release_all_streams();
  get_platform_context()->exit(static_cast<int32_t>(exit_code));
}
