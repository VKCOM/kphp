#include "runtime-headers.h"


task_t<void> k_main() noexcept  {
  co_await script_init();
  co_return;
}
