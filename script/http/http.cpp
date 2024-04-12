#include "runtime-headers.h"

void work(const string & str) {
  mixed arr;
  arr.push_back(str);
  arr.push_back(str.to_numeric());
}

bool is_http_query() {
  string type = PhpScriptMutableGlobals::current().get_superglobals().v$_SERVER.get_value(string("QUERY")).to_string();
  return type == string("http");
}

task_t<void> src() {
  if (!is_http_query()) {
    php_error("echo component can process only http query");
    co_return;
  }
  for (auto i : PhpScriptMutableGlobals::current().get_superglobals().v$_POST) {
    php_notice("%s -> %s", i.get_string_key().c_str(), i.get_value().to_string().c_str());
  }

  auto id = co_await f$component_client_send_query(string("echo"), string("hello, echo"));
  string res = co_await f$component_client_get_result(id);
  if (res != string("hello, echo")) {
    php_error("echo component give wrong answer \"%s\"", res.c_str());
    co_return;
  }
  php_notice("echo return \"%s\"", res.c_str());
  mixed result;
  result.set_value(string("header"), string("world"));
  result.set_value(string("body"), string("empty"));
  f$echo(f$json_encode(result, false).val());
}


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  co_await src();
  co_await finish(0);
  co_return;
}
