#include "runtime-headers.h"


task_t<void> f$src() noexcept {
  co_await init_superglobals();
  int64_t id = f$open(string("out"));
  co_await f$send(id, v$_POST);
  string str = co_await f$read(id);
  f$echo(str);
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
