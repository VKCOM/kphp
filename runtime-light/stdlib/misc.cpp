#include "runtime-light/stdlib/misc.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/panic.h"
#include "runtime-light/utils/json-functions.h"
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

task_t<void> parse_input_query(QueryType query_type) {
  ComponentState & ctx = *get_component_context();
  php_assert(ctx.standard_stream == 0);
  co_await wait_incoming_query_t{};
  ctx.standard_stream = ctx.incoming_pending_queries.front();
  ctx.incoming_pending_queries.pop_front();
  ctx.opened_streams[ctx.standard_stream] = NotBlocked;

  if (query_type == HTTP) {
    auto [buffer, size] = co_await read_all_from_stream(ctx.standard_stream);
    init_http_superglobals(buffer, size);
    get_platform_context()->allocator.free(buffer);
  } else if (query_type == COMPONENT) {
    // Processing takes place in the calling function
  } else {
    php_critical_error("unexpected query type %d in parse_input_query", query_type);
  }
  co_return ;
}

task_t<void> finish(int64_t exit_code, bool from_exit) {
  (void) from_exit;
  (void) exit_code;
  //todo:k2 use exit_code
  ComponentState &ctx = *get_component_context();
  if (ctx.standard_stream == 0) {
    co_return;
  }
  int ob_total_buffer = ob_merge_buffers();
  Response &response = ctx.response;
  auto &buffer = response.output_buffers[ob_total_buffer];
  bool ok = co_await write_all_to_stream(ctx.standard_stream, buffer.c_str(), buffer.size());
  if (!ok) {
    php_warning("cannot write component result to input stream %lu", ctx.standard_stream);
  }
  free_all_descriptors();
  ctx.poll_status = PollStatus::PollFinishedOk;
  co_return;
}

task_t<void> f$testyield() {
  co_await test_yield_t{};
}

void f$check_shutdown() {
  const PlatformCtx & ptx = *get_platform_context();
  if (get_platform_context()->please_graceful_shutdown.load()) {
    php_notice("script was graceful shutdown");
    ptx.abort();
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