// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>
#include <unordered_map>

#include "runtime/critical_section.h"

/**
 * STL unordered_map wrapper which is safe to use in KPHP runtime environment.
 * All modifiable operations are guarded with dl::CriticalSectionGuard.
 */
template<typename Key, typename Value>
class SignalSafeHashtable {
public:
  template<typename S>
  bool insert(const Key& key, S&& value) noexcept {
    static_assert(std::is_same_v<std::remove_cv_t<Value>, std::remove_cv_t<S>>);

    dl::CriticalSectionGuard guard;
    auto res = ht.insert(std::make_pair(key, std::forward<S>(value)));
    return res.second;
  }

  template<typename S>
  bool insert_or_assign(const Key& key, S&& value) noexcept {
    static_assert(std::is_same_v<std::remove_cv_t<Value>, std::remove_cv_t<S>>);

    dl::CriticalSectionGuard guard;
    auto res = ht.template insert_or_assign(key, std::forward<S>(value));
    return res.second;
  }

  Value extract(const Key& key) noexcept {
    dl::CriticalSectionGuard guard;
    auto item = ht.extract(key);
    if (item.empty()) {
      return {};
    }
    return std::move(item.mapped());
  }

  bool erase(const Key& key) noexcept {
    dl::CriticalSectionGuard guard;
    return ht.erase(key);
  }

  Value* get(const Key& key) noexcept {
    dl::CriticalSectionGuard guard;
    auto it = ht.find(key);
    if (it == ht.end()) {
      return nullptr;
    }
    return &it->second;
  }

  auto begin() const noexcept {
    return ht.begin();
  }

  auto end() const noexcept {
    return ht.end();
  }

  void clear() {
    dl::CriticalSectionGuard guard;
    ht.clear();
  }

private:
  std::unordered_map<Key, Value> ht;
};
