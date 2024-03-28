#include "runtime-headers.h"

task_t<void> f$src() noexcept  {
  // init_superglobals();
  // auto res = co_await read_all_from_stream(get_component_context()->standard_stream);
  // f$echo(res);
  co_await f$dummy_echo();
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
