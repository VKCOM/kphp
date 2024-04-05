#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  while (true) {
    string query = co_await f$component_server_get_query();
    if (query == string("profile")) {
      co_await f$component_server_send_result(string("db send profile"));
    } else if (query == string("feed")) {
      co_await f$component_server_send_result(string("db send feed"));
    } else if (query == string("music")) {
      co_await f$component_server_send_result(string("db send music"));
    } else if (query == string("exit")) {
      co_await f$component_server_send_result(string("strange query"));
      break;
    }
  }
  co_await finish(0);
  co_return;
}
