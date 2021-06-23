// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/ifi.h"

#include "common/wrappers/string_view.h"

// A space-efficient map that has value semantics,
// it doesn't do any heap allocations.
//
// For every element it uses 2+16 bytes (2 for ifi_mask plus 16 for string_view).
// Therefore, MaxSize=4 results in 72 bytes (relatively cheap to copy it).
//
// Note that it does the linear search through keys.
// It makes it impractical for the number of keys that exceeds ~10.
template<int MaxSize, is_func_id_t DefaultValue = static_cast<is_func_id_t>(ifi_any_type)>
class IfiMap {
  static_assert(MaxSize <= 16, "big ifi maps should not be used");

private:

  int count_ = 0;
  std::array<uint16_t, MaxSize> masks_ = {};
  std::array<vk::string_view, MaxSize> keys_ = {};

  void set_kv(int index, vk::string_view key, is_func_id_t mask) {
    masks_[index] = static_cast<uint16_t>(mask);
    keys_[index] = key;
  }

  void push_kv(vk::string_view key, is_func_id_t mask) {
    int index = count_;
    ++count_;
    set_kv(index, key, mask);
  }

public:

  size_t size() const noexcept { return count_; }

  void clear() noexcept { count_ = 0; }

  void update_all(is_func_id_t mask) {
    for (int i = 0; i < count_; ++i) {
      masks_[i] = static_cast<uint16_t>(mask);
    }
  }

  void insert(vk::string_view key, is_func_id_t mask) {
    if (count_ == keys_.size()) {
      return;
    }
    for (int i = 0; i < count_; ++i) {
      if (keys_[i] == key) {
        set_kv(i, key, mask);
        return;
      }
    }
    push_kv(key, mask);
  }

  is_func_id_t get(vk::string_view key) {
    for (int i = 0; i < count_; ++i) {
      if (keys_[i] == key) {
        return static_cast<is_func_id_t>(masks_[i]);
      }
    }
    return DefaultValue;
  }
};
