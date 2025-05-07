#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-light/stdlib/instance-cache/request-cache.h"

struct InstanceCacheState : private vk::not_copyable {
  kphp::instance_cache::RequestCache request_cache;
  static InstanceCacheState& get() noexcept;
};
