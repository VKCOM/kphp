#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"

struct KmlInferenceContext final : vk::not_copyable {
  std::byte* mutable_buffer_in_worker = nullptr;

  static KmlInferenceContext& get() noexcept;
};
