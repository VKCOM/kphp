// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/utils/timer.h"

uint64_t set_timer_without_callback(int64_t timeout_ns) {
  const PlatformCtx & ptx = *get_platform_context();
  uint64_t nanoseconds = timeout_ns;
  uint64_t timer_d = 0;
  SetTimerResult res = ptx.set_timer(&timer_d, nanoseconds);
  if (res != SetTimerOk) {
    php_warning("timer limit exceeded");
    return BAD_PLATFORM_DESCRIPTOR;
  }
  php_debug("set up timer %lu for %ld ns", timer_d, timeout_ns);
  register_timer(timer_d);
  return timer_d;
}
