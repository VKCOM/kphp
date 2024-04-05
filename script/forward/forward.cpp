#include "runtime-headers.h"


task_t<void> k_main() noexcept {
  co_await parse_input_query();
  string query = get_component_context()->superglobals.v$_RAW_QUERY;
  int64_t id = co_await f$component_client_send_query(string("out"), query);
  if (id == v$COMPONENT_ERROR) {
    co_return;
  }
  string result = co_await f$component_client_get_result(id);
  co_await f$component_server_send_result(result);
  co_await finish(0);
  co_return ;
}
