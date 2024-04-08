#include "runtime-headers.h"

task_t<string> ask_server(const string & ask) {
  int64_t id = co_await f$component_client_send_query(string("server"), ask);
  if (id == v$COMPONENT_ERROR) {
    php_notice("component error");
    co_return string();
  }
  string result = co_await f$component_client_get_result(id);
  co_return result;
}

task_t<mixed> get_data() {
  mixed data;

  string profile = co_await ask_server(string("profile"));
  data.set_value(string("profile"), profile);

//  string feed = co_await ask_server(string("feed"));
//  data.set_value(string("feed"), feed);
//
//  string music = co_await ask_server(string("music"));
//  data.set_value(string("music"), music);
//
  co_await ask_server(string("exit"));
  co_return data;
}

string data_check(const mixed & data) {
  string profile = data.get_value(string("profile")).as_string();
  if (profile != string("db send profile server process profile")) {
    php_notice("got %s", profile.c_str());
    return string("erro");
  }

//  string feed = data.get_value(string("feed")).as_string();
//  if (feed != string("db send feed server process feed")) {
//    php_notice("got %s", feed.c_str());
//    return string("erro");
//  }
//
//  string music = data.get_value(string("music")).as_string();
//  if (music != string("db send music server process music")) {
//    php_notice("got %s", music.c_str());
//    return string("erro");
//  }

  return string("okay");
}

task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  mixed data = co_await get_data();
  string res = data_check(data);
  f$echo(res);
  co_await finish(0);
  co_return;
}
