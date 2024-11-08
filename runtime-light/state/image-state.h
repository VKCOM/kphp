// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/string/string-state.h"

struct ImageState final : private vk::not_copyable {
  RuntimeAllocator allocator;

  char *c_linear_mem{nullptr};
  RpcImageState rpc_image_state{};
  StringImageState string_image_state{};

  ImageState() noexcept
    : allocator(INIT_IMAGE_ALLOCATOR_SIZE, 0) {}

  static const ImageState &get() noexcept {
    return *k2::image_state();
  }

  static ImageState &get_mutable() noexcept {
    return *const_cast<ImageState *>(k2::image_state());
  }

private:
  static constexpr auto INIT_IMAGE_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
