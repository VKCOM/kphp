#include "runtime-headers.h"


task_t<void> k_main() noexcept {
  co_await init();
  int64_t id = f$open(string("out"));
  co_await f$send(id, get_component_context()->superglobals.v$_POST.as_string());
  string str = co_await f$read(id);
  f$echo(str);
  co_await finish(0);
  co_return ;
}
