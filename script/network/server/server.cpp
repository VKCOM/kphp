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
  php_notice("start server");
  while (true) {
    co_await init();

    string query = f$component_server_get_query();
    string result;
    if (query == string("profile")) {
      php_notice("server process profile");
      result = co_await ask_db(string("profile"));
      result.append(" server process profile");
      co_await f$component_server_send_result(result);
    } else if (query == string("feed")) {
      php_notice("server process feed");
      result = co_await ask_db(string("feed"));
      result.append(" server process feed");
      co_await f$component_server_send_result(result);
    } else if (query == string("music")) {
      php_notice("server process feed");
      result = co_await ask_db(string("music"));
      result.append("server process music");
      co_await f$component_server_send_result(result);
    } else if (query == string("exit")) {
      result = co_await ask_db(query);
      co_await f$component_server_send_result(result.append(string("strange query")));
      break;
    }
  }


  co_await finish(0);
  co_return;
}
