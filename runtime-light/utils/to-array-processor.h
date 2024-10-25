// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "runtime/null_coalesce.h"

class ShapeKeyDemangle : vk::not_copyable {
public:
  friend class vk::singleton<ShapeKeyDemangle>;

  void init(std::unordered_map<std::int64_t, std::string_view> &&shape_keys_storage) noexcept {
    inited_ = true;
    shape_keys_storage_ = std::move(shape_keys_storage);
  }

  std::string_view get_key_by(std::int64_t tag) const noexcept {
    php_assert(inited_);
    auto key_it = shape_keys_storage_.find(tag);
    php_assert(key_it != shape_keys_storage_.end());
    return key_it->second;
  }

private:
  ShapeKeyDemangle() = default;

  bool inited_{false};
  std::unordered_map<std::int64_t, std::string_view> shape_keys_storage_;
};
