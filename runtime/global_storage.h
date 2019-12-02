#pragma once

#include <type_traits>

#include "common/mixin/not_copyable.h"

#include "runtime/kphp_core.h"

template<class T>
class GlobalStorage : vk::not_copyable {
public:
  GlobalStorage() noexcept :
    data_(reinterpret_cast<T *>(&storage_)) {
    hard_reset();
  }

  void init(int64_t query_tag = 0) noexcept {
    query_tag_ = query_tag;
  }

  int64_t get_query_tag() const noexcept {
    return query_tag_;
  }

  T *operator->() noexcept {
    return data_;
  }

  void hard_reset() noexcept {
    hard_reset_var(*data_);
    --query_tag_;
  }

private:
  std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
  T *data_{nullptr};
  int64_t query_tag_{0};
};

template<typename T>
class SingletonStorage {
public:
  static GlobalStorage<T> &get() noexcept{
    static GlobalStorage<T> obj;
    return obj;
  }
};
