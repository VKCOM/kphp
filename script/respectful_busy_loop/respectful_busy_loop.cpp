#include "runtime-headers.h"

mixed f$start_transaction() noexcept {
  mixed a;
  a.set_value(1, string("kek"));
  a.set_value(2, string("lol"));
  return  a;
}

task_t<void> f$hard_work() noexcept {
  while (true) {
    co_await f$testyield();
    mixed res = f$start_transaction();
    f$check_shutdown();
    f$var_dump(res);
  }
}

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY_TYPE")).to_string();
  return type == string("component");
}

task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  if (!is_component_query()) {
    php_error("respectful component can process only component query");
    co_return;
  }
  co_await f$hard_work();
  co_await finish(0, false);
  co_return ;
}
