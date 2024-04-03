#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await init();
  co_await finish(0);
  co_return;
}
