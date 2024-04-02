#include "runtime-headers.h"


task_t<void> f$src() noexcept  {
  co_await init_superglobals();
  int64_t id = f$open(string("out"));
  co_await f$send(id, string("hello"));
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
