#include "runtime-light/utils/timer.h"

void set_timer_impl(int64_t timeout_ms, on_timer_callback_t && callback) {
  const PlatformCtx & ptx = *get_platform_context();
  ComponentState & ctx = *get_component_context();
  uint64_t nanoseconds = static_cast<uint64_t>(timeout_ms * 1e6);
  uint64_t timer_d = 0;
  SetTimerResult res = ptx.set_timer(&timer_d, nanoseconds);
  if (res != SetTimerOk) {
    php_warning("timer limit exceeded");
    return;
  }
  php_debug("set up timer %lu for %lu ms", timer_d, timeout_ms);

  ctx.timer_callbacks[timer_d] = callback;
}

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
  ctx.timer_callbacks[timer_d] = [](){};
  return timer_d;
}