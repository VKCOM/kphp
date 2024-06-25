#include "runtime-light/utils/timer.h"

uint64_t set_timer_without_callback(double timeout_s) {
  const PlatformCtx & ptx = *get_platform_context();
  ComponentState & ctx = *get_component_context();
  uint64_t nanoseconds = static_cast<uint64_t>(timeout_s * 1e9);
  uint64_t timer_d = 0;
  SetTimerResult res = ptx.set_timer(&timer_d, nanoseconds);
  if (res != SetTimerOk) {
    php_warning("timer limit exceeded");
    return -1;
  }
  php_debug("set up timer %lu for %f s", timer_d, timeout_s);
  ctx.opened_descriptors[timer_d] = DescriptorRuntimeStatus::Timer;
  return timer_d;
}