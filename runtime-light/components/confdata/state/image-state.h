// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-light/k2-platform/k2-api.h"

struct ImageState final : private vk::not_copyable {
  ImageState() noexcept = default;
  static auto get() noexcept -> const ImageState&;
  static auto get_mutable() noexcept -> ImageState&;
};

inline auto ImageState::get() noexcept -> const ImageState& {
  return *k2::image_state();
}

inline auto ImageState::get_mutable() noexcept -> ImageState& {
  return const_cast<ImageState&>(ImageState::get());
}
