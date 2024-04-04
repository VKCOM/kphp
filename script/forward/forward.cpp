#include "runtime-headers.h"


task_t<void> k_main() noexcept {
  co_await init();
  string query = f$component_server_get_query();
  int64_t id = co_await f$component_client_send_query(string("out"), query);
  if (id == v$COMPONENT_ERROR) {
    co_return;
  }
  string result = co_await f$component_client_get_result(id);
  co_await f$component_server_send_result(result);
  co_await finish(0);
  co_return ;
}
