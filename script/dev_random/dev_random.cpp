#include "runtime-headers.h"

task_t<void> call() {
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("out"),  string("hello"));
}

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY")).to_string();
  return type == string("component");
}


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("dev-random component can process only component query");
    co_return;
  }
  co_await call();
  co_await finish(0);
  co_return ;
}
