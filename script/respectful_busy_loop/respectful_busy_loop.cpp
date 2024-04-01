#include "runtime-headers.h"

mixed f$start_transaction() noexcept {
  mixed a;
  a.set_value(1, string("kek"));
  a.set_value(2, string("lol"));
  f$check_shutdown();
  return  a;
}

task_t<void> f$hard_work() noexcept {
  while (true) {
    co_await f$yield();
    f$echo(f$start_transaction().as_string());
  }
}

task_t<void> f$src() noexcept  {
  co_await f$hard_work();
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
