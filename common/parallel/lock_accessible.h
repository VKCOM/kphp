// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mutex>

namespace vk {

template<typename T, typename mutex_type>
class lock_accessible {
private:
  class lock_accessible_wrapper : std::unique_lock<mutex_type> {
  public:
    explicit lock_accessible_wrapper(lock_accessible& this_) noexcept
        : std::unique_lock<mutex_type>(this_.lock_),
          obj_(this_.obj_) {}

    T* operator->() noexcept {
      return &obj_;
    }

  private:
    T& obj_;
  };

public:
  lock_accessible_wrapper acquire_lock() noexcept {
    return lock_accessible_wrapper{*this};
  }

  lock_accessible_wrapper operator->() noexcept {
    return acquire_lock();
  }

private:
  // TODO variadic construct?
  T obj_;
  mutex_type lock_;
};

} // namespace vk
