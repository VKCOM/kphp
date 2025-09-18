#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/kml/kphp_ml.h"

struct KmlModelsContext final : vk::not_copyable {
  const char* kml_directory = nullptr;
  kphp::stl::unordered_map<uint64_t, kphp_ml::MLModel, kphp::memory::platform_allocator> loaded_models;
  unsigned int max_mutable_buffer_size = 0;

  static const KmlModelsContext& get() noexcept;
  static KmlModelsContext& get_mutable() noexcept;
};
