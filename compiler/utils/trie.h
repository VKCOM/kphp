// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cassert>
#include <memory>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/optional.h"

template<typename T>
struct Trie : private vk::not_copyable {
  std::array<std::unique_ptr<Trie>, 256> next{};
  vk::optional<T> val;

  void add(const string &s, T val) {
    Trie *cur = this;

    for (char c : s) {
      if (cur->next[c] == nullptr) {
        cur->next[c] = std::make_unique<Trie>();
      }
      cur = cur->next[c].get();
    }

    assert(!cur->val);
    cur->val = std::move(val);
  }

  vk::optional<T> get_deepest(const char *s) {
    auto *best = this;

    for (auto cur = this; *s && cur; ++s) {
      cur = cur->next[static_cast<uint8_t>(*s)].get();
      if (cur && cur->val) {
        best = cur;
      }
    }

    return best->val;
  }
};

