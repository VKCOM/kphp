// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>

namespace vk {

template<class T>
class copyable_atomic : std::atomic<T> {
public:
  using std::atomic<T>::atomic;
  using std::atomic<T>::operator=;
  using std::atomic<T>::store;
  using std::atomic<T>::load;
  using std::atomic<T>::exchange;
  using std::atomic<T>::operator T;
  using std::atomic<T>::compare_exchange_strong;
  using std::atomic<T>::compare_exchange_weak;

  copyable_atomic(const copyable_atomic &other) :
    std::atomic<T>(other.load()) {
  }

  copyable_atomic& operator=(copyable_atomic other) {
    *this = other.load();
    return *this;
  }

  // Add other operators if it is required
};

} // namespace vk
