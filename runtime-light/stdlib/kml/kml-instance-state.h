#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/stdlib/kml/kml-inference-context.h"
#include "runtime-common/stdlib/kml/kml-models-context.h"
#include "runtime-common/stdlib/kml/kphp_ml_init.h"

struct KmlInstanceState final : private vk::not_copyable {
  KmlInferenceContext kml_inference_context;

  KmlInstanceState() noexcept {
    init_kphp_ml_runtime_in_worker();
  }

  ~KmlInstanceState() noexcept {
    kphp::memory::platform_allocator<char>{}.deallocate(kml_inference_context.mutable_buffer_in_worker, KmlModelsContext::get().max_mutable_buffer_size);
  }

  static KmlInstanceState& get() noexcept;
};
