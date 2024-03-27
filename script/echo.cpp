#include "runtime-headers.h"

task_t<void> f$src() noexcept  {
  //  init_superglobals();
  co_await f$dummy_echo();
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
