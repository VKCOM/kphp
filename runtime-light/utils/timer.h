#pragma once

#include <functional>

#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/component/component.h"

using on_timer_callback_t = std::function<void()>;

void set_timer_impl(int64_t timeout_ms, on_timer_callback_t && callback);

template<typename CallBack>
void f$set_timer(int64_t timeout, CallBack && callback) {
  set_timer_impl(timeout, on_timer_callback_t(std::forward<CallBack>(callback)));
}