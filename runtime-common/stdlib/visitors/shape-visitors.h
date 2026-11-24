// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

template<template<class> class Allocator>
struct ShapeKeyDemangle final : vk::not_copyable {
  using storage_type = kphp::stl::unordered_map<int64_t, std::string_view, Allocator>;

  ShapeKeyDemangle() = default;

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

  static const ShapeKeyDemangle& get() noexcept;

private:
  bool inited_{};
  storage_type shape_keys_storage_;
};
