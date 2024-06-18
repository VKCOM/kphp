// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "compiler/debug.h"
#include "compiler/inferring/key.h"
#include "compiler/kphp_assert.h"

class MultiKey {
  DEBUG_STRING_METHOD { return to_string(); }

private:
  std::vector<Key> keys_;
  static std::vector<MultiKey> any_key_vec;

public:
  using iterator = std::vector<Key>::const_iterator;
  using reverse_iterator = std::vector<Key>::const_reverse_iterator;

  MultiKey() = default;
  MultiKey(const MultiKey &multi_key) = default;
  MultiKey &operator=(const MultiKey &multi_key) = default;

  explicit MultiKey(std::vector<Key> keys) :
    keys_(std::move(keys)) {
  }

  void push_back(const Key &key) {
    keys_.push_back(key);
  }

  void push_front(const Key &key) {
    keys_.insert(keys_.begin(), key);
  }

  inline uint32_t depth() const {
    return static_cast<uint32_t>(keys_.size());
  }

  iterator begin() const {
    return keys_.begin();
  }

  iterator end() const {
    return keys_.end();
  }

  reverse_iterator rbegin() const {
    return keys_.rbegin();
  }

  reverse_iterator rend() const {
    return keys_.rend();
  }

  bool empty() const {
    return keys_.empty();
  }

  static void init_static() {
    for (uint32_t i = 0; i < 10; i++) {
      any_key_vec.push_back(MultiKey{std::vector<Key>{i, Key::any_key()}});
    }
  }

  static const MultiKey &any_key(uint32_t depth) {
    if (depth < any_key_vec.size()) {
      return any_key_vec[depth];
    }
    kphp_fail();
  }

  std::string to_string() const;
};
