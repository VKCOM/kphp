#include "runtime-light/stdlib/misc.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/panic.h"
#include "runtime-light/coroutine/awaitable.h"

static int ob_merge_buffers() {
  Response &response = get_component_context()->response;
  php_assert (response.current_buffer >= 0);
  int ob_first_not_empty = 0;
  while (ob_first_not_empty < response.current_buffer && response.output_buffers[ob_first_not_empty].size() == 0) {
    ob_first_not_empty++;
  }
  for (int i = ob_first_not_empty + 1; i <= response.current_buffer; i++) {
    response.output_buffers[ob_first_not_empty].append(response.output_buffers[i].c_str(), response.output_buffers[i].size());
  }
  return ob_first_not_empty;
}

task_t<void> finish(int64_t exit_code) {
  int ob_total_buffer = ob_merge_buffers();

  const PlatformCtx & pt_ctx = *get_platform_context();
  ComponentState &ctx = *get_component_context();
  Response &response = ctx.response;
  auto &buffer = response.output_buffers[ob_total_buffer];


  uint64_t stream_d = ctx.standard_stream;
  StreamStatus status;
  size_t processed = 0;
  while (processed != buffer.size()) {
    GetStatusResult res = pt_ctx.get_stream_status(stream_d, &status);
    if (res != GetStatusResult::GetStatusOk) {
      get_component_context()->poll_status = PollStatus::PollFinished;
      co_return;
    }
    if (status.write_status == IOStatus::IOBlocked) {
      ctx.awaited_stream = stream_d;
      co_await platform_switch_t{};
    } else if (status.write_status == IOStatus::IOClosed) {
      get_component_context()->poll_status = PollStatus::PollFinished;
      co_return;
    }
    size_t processed = pt_ctx.write(get_component_context()->standard_stream, buffer.size() - processed, buffer.c_str() + processed);
  }
  get_component_context()->poll_status = PollStatus::PollFinished;
  co_return;
}

task_t<void> f$exit(const mixed &v) {
  if (v.is_string()) {
    Response &response = get_component_context()->response;
    response.output_buffers[response.current_buffer] << v;
    co_await finish(0);
  } else {
    co_await finish(v.to_int());
  }
  panic();
}


task_t<void> f$die(const mixed &v) {
  co_await f$exit(v);
}