// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>

#include "common/mixin/not_copyable.h"

template<typename T>
struct Trie : private vk::not_copyable {
  std::array<std::unique_ptr<Trie>, 256> next{};
  std::optional<T> val;

  void add(const std::string &s, T val) {
    Trie *cur = this;

    for (char c : s) {
      if (cur->next[c] == nullptr) {
        cur->next[c] = std::make_unique<Trie>();
      }
      cur = cur->next[c].get();
    }

    assert(!cur->val && "The symbol in variable 's' has already been added by the rule earlier");
    cur->val = std::move(val);
  }

  std::optional<T> get_deepest(const char *s) {
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

