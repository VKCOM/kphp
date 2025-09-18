#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"

struct KmlInferenceContext final : vk::not_copyable {
  char* mutable_buffer_in_worker = nullptr;

  static KmlInferenceContext& get() noexcept;
};
