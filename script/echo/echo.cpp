#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await script_init();
   f$echo(get_component_context()->superglobals.v$_POST.as_string());
  co_await finish(0);
  co_return ;
}
