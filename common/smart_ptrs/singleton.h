#pragma once

#include <type_traits>
#include <utility>

namespace vk {

template<typename T>
class singleton {
public:
  static T& get() {
    static T data;
    return data;
  }

  T *operator->() {
    return &get();
  }

  T &operator*() {
    return get();
  }

  template<typename Arg>
  decltype(std::declval<T>()[std::declval<Arg>()]) operator[](Arg &&arg) {
    return get()[std::forward<Arg>(arg)];
  }
};

} // namespace vk
