#include "runtime-headers.h"

void work(const string & str) {
  mixed arr;
  arr.push_back(str);
  arr.push_back(str.to_numeric());
}


task_t<void> k_main() noexcept  {
  co_await init();
  string str = f$component_server_get_query();
  work(str);
  f$echo(str);
  co_await finish(0);
  co_return;
}
