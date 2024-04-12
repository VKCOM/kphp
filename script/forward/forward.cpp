#include "runtime-headers.h"

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY")).to_string();
  return type == string("component");
}

task_t<void> k_main() noexcept {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("forward component can process only component query");
    co_return;
  }
  string query = co_await f$component_server_get_query();
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("out"), query);
  if (f$is_null(id)) {
    co_return;
  }
  string result = co_await f$component_client_get_result(id);
  co_await f$component_server_send_result(result);
  co_await finish(0);
  co_return ;
}
