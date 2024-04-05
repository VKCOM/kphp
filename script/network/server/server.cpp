#include "runtime-headers.h"

task_t<string> ask_db(const string & ask) {
  int64_t id = co_await f$component_client_send_query(string("db"), ask);
  if (id == v$COMPONENT_ERROR) {
    co_return string();
  }
  string result = co_await f$component_client_get_result(id);
  co_return result;
}

task_t<void> k_main() noexcept  {
  while (true) {
    string query = co_await f$component_server_get_query();
    string result;
    if (query == string("profile")) {
      result = co_await ask_db(string("profile"));
      co_await f$component_server_send_result(result.append(" server process profile"));
    } else if (query == string("feed")) {
      result = co_await ask_db(string("feed"));
      co_await f$component_server_send_result(result.append(" server process feed"));
    } else if (query == string("music")) {
      result = co_await ask_db(string("music"));
      co_await f$component_server_send_result(result.append(" server process music"));
    } else if (query == string("exit")) {
      co_await ask_db(query);
      break;
    } else {
      php_notice("strange query");
      co_await f$component_server_send_result(result.append(string(" strange query")));
    }
  }
  co_await finish(0);
  co_return;
}
