#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  int64_t id = co_await f$component_client_send_query(string("out"),  string("hello"));
  co_await finish(0);
  co_return ;
}
