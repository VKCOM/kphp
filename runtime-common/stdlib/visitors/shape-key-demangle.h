// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

struct ShapeKeyDemangle final : vk::not_copyable {
  using storage_type = kphp::stl::unordered_map<int64_t, std::string_view, kphp::memory::script_allocator>;

  ShapeKeyDemangle() = default;

  static const ShapeKeyDemangle& get() noexcept;

  void init(storage_type&& shape_keys_storage) noexcept {
    inited_ = true;
    shape_keys_storage_ = std::move(shape_keys_storage);
  }

  std::string_view get_key_by(int64_t tag) const noexcept {
    php_assert(inited_);
    auto key_it{shape_keys_storage_.find(tag)};
    php_assert(key_it != shape_keys_storage_.end());
    return key_it->second;
  }

private:
  bool inited_{};
  // Initialized once during startup.
  // In the legacy runtime, heap_replacer is active at that point, so storage is allocated from the heap; the map is never reset after switching to the script allocator.
  // In runtime-light, storage is allocated from the image allocator and never reset after switching to component or instance allocators.
  storage_type shape_keys_storage_;
};
