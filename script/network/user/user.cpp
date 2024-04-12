#include "runtime-headers.h"

task_t<class_instance<C$ComponentQuery>> send_request(const string & message) {
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("server"), message);
  co_return id;
}

task_t<string> wait_request(class_instance<C$ComponentQuery> id) {
  string result = co_await f$component_client_get_result(id);
  co_return result;
}


task_t<string> ask_server(const string & ask) {
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("server"), ask);
  if (f$is_null(id)) {
    php_notice("component error");
    co_return string();
  }
  string result = co_await f$component_client_get_result(id);
  co_return result;
}

task_t<mixed> get_data() {
  mixed data;
  class_instance<C$ComponentQuery> pid = co_await send_request(string("profile"));
  class_instance<C$ComponentQuery> fid = co_await send_request(string("feed"));
  class_instance<C$ComponentQuery> mid = co_await send_request(string("music"));

  string profile = co_await wait_request(pid);
  data.set_value(string("profile"), profile);
  php_debug("user get profile");
  string feed = co_await wait_request(fid);
  data.set_value(string("feed"), feed);
  php_debug("user get feed");
  string music = co_await wait_request(mid);
  data.set_value(string("music"), music);
  php_debug("user get music");

  co_await send_request(string("exit"));
  co_return data;
}

string data_check(const mixed & data) {
  string profile = data.get_value(string("profile")).as_string();
  if (profile != string("db send profile server process profile")) {
    php_notice("got %s", profile.c_str());
    return string("erro");
  }

  string feed = data.get_value(string("feed")).as_string();
  if (feed != string("db send feed server process feed")) {
    php_notice("got %s", feed.c_str());
    return string("erro");
  }

  string music = data.get_value(string("music")).as_string();
  if (music != string("db send music server process music")) {
    php_notice("got %s", music.c_str());
    return string("erro");
  }

  return string("okay");
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
  mixed data = co_await get_data();
  string res = data_check(data);
  co_await f$component_server_send_result(res);
  co_await finish(0);
  co_return;
}
