// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"

struct ShapeKeyDemangle final : vk::not_copyable {
  ShapeKeyDemangle() = default;

  void init(kphp::stl::unordered_map<int64_t, std::string_view, kphp::memory::script_allocator> &&shape_keys_storage) noexcept {
    inited_ = true;
    shape_keys_storage_ = std::move(shape_keys_storage);
  }

  std::string_view get_key_by(int64_t tag) const noexcept {
    php_assert(inited_);
    auto key_it = shape_keys_storage_.find(tag);
    php_assert(key_it != shape_keys_storage_.end());
    return key_it->second;
  }

private:
  bool inited_{};
  kphp::stl::unordered_map<int64_t, std::string_view, kphp::memory::script_allocator> shape_keys_storage_;
};
