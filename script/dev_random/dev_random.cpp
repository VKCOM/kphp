#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await init();
  int64_t id = f$open(string("out"));
  co_await f$send(id, string("hello"));
  co_await finish(0);
  co_return ;
}
