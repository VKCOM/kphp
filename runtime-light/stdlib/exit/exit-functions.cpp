// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/exit/exit-functions.h"

#include <cstdint>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

task_t<void> f$exit(const mixed &v) noexcept { // TODO: make it synchronous
  auto &instance_st{InstanceState::get()};

  int64_t exit_code{};
  if (v.is_string()) {
    Response &response{instance_st.response};
    response.output_buffers[response.current_buffer] << v;
  } else if (v.is_int()) {
    int64_t v_code{v.to_int()};
    // valid PHP exit codes: [0..254]
    exit_code = v_code >= 0 && v_code <= 254 ? v_code : 1;
  } else {
    exit_code = 1;
  }
  instance_st.poll_status =
    instance_st.poll_status != k2::PollStatus::PollFinishedError && exit_code == 0 ? k2::PollStatus::PollFinishedOk : k2::PollStatus::PollFinishedError;
  co_await instance_st.run_instance_epilogue();
  k2::exit(static_cast<int32_t>(exit_code));
}
