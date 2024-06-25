#include "runtime-light/utils/timer.h"

uint64_t set_timer_without_callback(int64_t timeout_ns) {
  const PlatformCtx & ptx = *get_platform_context();
  ComponentState & ctx = *get_component_context();
  uint64_t nanoseconds = timeout_ns;
  uint64_t timer_d = 0;
  SetTimerResult res = ptx.set_timer(&timer_d, nanoseconds);
  if (res != SetTimerOk) {
    php_warning("timer limit exceeded");
    return -1;
  }
  php_debug("set up timer %lu for %ld ns", timer_d, timeout_ns);
  ctx.opened_descriptors[timer_d] = DescriptorRuntimeStatus::Timer;
  return timer_d;
}
