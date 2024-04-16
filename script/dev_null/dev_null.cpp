#include "runtime-headers.h"

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY_TYPE")).to_string();
  return type == string("component");
}

task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("dev-null component can process only component query");
    co_return;
  }
  string query = co_await f$component_server_get_query();
  co_await finish(0, false);
  co_return;
}
