#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await parse_input_query();
  co_await finish(0);
  co_return;
}
