#include "runtime-headers.h"

task_t<void> call() {
  class_instance<C$ComponentQuery> id = co_await f$component_client_send_query(string("out"),  string("hello"));
}


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  co_await call();
  co_await finish(0);
  co_return ;
}
