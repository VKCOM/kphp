#include "runtime-headers.h"

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY")).to_string();
  return type == string("component");
}

task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("component can process only component query");
    co_return;
  }
  while (true) {
    string query = co_await f$component_server_get_query();
    if (query == string("profile")) {
      co_await f$component_server_send_result(string("db send profile"));
    } else if (query == string("feed")) {
      co_await f$component_server_send_result(string("db send feed"));
    } else if (query == string("music")) {
      co_await f$component_server_send_result(string("db send music"));
    } else if (query == string("exit")) {
      break;
    } else {
      php_notice("strange query");
      co_await f$component_server_send_result(string("strange query"));
    }
  }
  co_await finish(0);
  co_return;
}
