#include "runtime-headers.h"


task_t<void> f$src() noexcept  {
  co_await init_superglobals();
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
