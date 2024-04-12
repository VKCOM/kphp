#include "runtime-headers.h"

void work(const string & str) {
  mixed arr;
  arr.push_back(str);
  arr.push_back(str.to_numeric());
}

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY_TYPE")).to_string();
  return type == string("component");
}


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("echo component can process only component query");
    co_return;
  }
  string str = co_await f$component_server_get_query();
  work(str);
  co_await f$component_server_send_result(str);
  co_await finish(0);
  co_return;
}
