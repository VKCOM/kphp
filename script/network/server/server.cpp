#include "runtime-headers.h"

task_t<class_instance<C$ComponentQuery>> send_request(const string & message) {
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("db"), message);
  co_return id;
}

task_t<string> wait_request(class_instance<C$ComponentQuery> id) {
  string result = co_await f$component_client_get_result(id);
  co_return result;
}


task_t<string> ask_db(const string & ask) {
  class_instance<C$ComponentQuery> id = co_await send_request(ask);
  if (f$is_null(id)) {
    php_notice("component error");
    co_return string();
  }
  string result = co_await wait_request(id);
  co_return result;
}

bool is_component_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY_TYPE")).to_string();
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
      co_await send_request(query);
      break;
    } else {
      php_notice("strange query \"%s\"", query.c_str());
      co_await f$component_server_send_result(result.append(string(" strange query")));
    }
  }
  co_await finish(0, false);
  co_return;
}
