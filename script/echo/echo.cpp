#include "runtime-headers.h"


task_t<void> f$src() noexcept  {
   co_await init_superglobals();
   f$echo(get_component_context()->superglobals.v$POST);
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
