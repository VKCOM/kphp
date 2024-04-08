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

task_t<void> parse_input_query() {
  ComponentState & ctx = *get_component_context();
  php_assert(ctx.standard_stream == 0);
  co_await wait_input_query_t{};
  ctx.standard_stream = ctx.pending_queries.front();
  ctx.pending_queries.pop();
  ctx.processed_queries[ctx.standard_stream] = NotBlocked;
  auto [buffer, size] = co_await read_all_from_stream(ctx.standard_stream);
  init_superglobals(buffer, size);
  get_platform_allocator()->free(buffer);
  co_return ;
}

task_t<void> finish(int64_t exit_code) {
  ComponentState &ctx = *get_component_context();
  if (ctx.standard_stream == 0) {
    co_return;
  }
  int ob_total_buffer = ob_merge_buffers();
  Response &response = ctx.response;
  auto &buffer = response.output_buffers[ob_total_buffer];

  co_await write_all_to_stream(ctx.standard_stream, buffer.c_str(), buffer.size());
  free_all_descriptors();
  ctx.poll_status = PollStatus::PollFinished;
  co_return;
}

task_t<void> f$testyield() {
  co_await test_yield_t{};
}

void f$check_shutdown() {
  if (get_platform_context()->please_graceful_shutdown.load()) {
    php_notice("script was graceful shutdown");
    siglongjmp(get_component_context()->exit_tag, 1);
  }
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