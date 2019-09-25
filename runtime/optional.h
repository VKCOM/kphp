#pragma once

#include "common/type_traits/is_constructible.h"
#include "common/type_traits/list_of_types.h"

#include "runtime/php_assert.h"

template<class T>
class OrFalse;

template<class T>
struct is_or_false : std::false_type {
};

template<class T>
struct is_or_false<OrFalse<T>> : std::true_type {
};

template<class T>
class OrFalse {
public:
  static_assert(!vk::list_of_types<bool, var>::template in_list<T>(), "Usages OrFalse<bool> and OrFalse<var> are forbidden");
  static_assert(!is_or_false<T>::value, "Usage OrFalse<OrFalse> is forbidden");

  using InnerType = T;

  T value;
  bool bool_value;

  OrFalse() noexcept :
    value(),
    bool_value() {
  }

  OrFalse(bool x) noexcept :
    value(),
    bool_value(x) {
    php_assert(!x);
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(T1 &&x) noexcept :
    value(std::forward<T1>(x)),
    bool_value(true) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(const OrFalse<T1> &other) noexcept :
    value(other.value),
    bool_value(other.bool_value) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(OrFalse<T1> &&other) noexcept :
    value(std::move(other.value)),
    bool_value(other.bool_value) {
    other.bool_value = false;
  }

  OrFalse &operator=(bool x) noexcept {
    php_assert(!x);
    value = T();
    bool_value = x;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(const OrFalse<T1> &other) noexcept {
    value = T(other.value);
    bool_value = other.bool_value;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(OrFalse<T1> &&other) noexcept {
    value = T(std::move(other.value));
    bool_value = other.bool_value;
    other.bool_value = false;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(T1 &&x) noexcept {
    value = T(std::forward<T1>(x));
    bool_value = true;
    return *this;
  }

  OrFalse &operator=(T &&x) noexcept {
    value = std::move(x);
    bool_value = true;
    return *this;
  }

  OrFalse &operator=(const T &x) noexcept {
    value = x;
    bool_value = true;
    return *this;
  }

  T &ref() noexcept {
    bool_value = true;
    return value;
  }

  T &val() noexcept {
    return value;
  }

  const T &val() const noexcept {
    return value;
  }
};

template<class T>
using enable_if_t_is_or_false = std::enable_if_t<is_or_false<std::decay_t<T>>::value>;

template<class T, class T2>
using enable_if_t_is_or_false_t2 = std::enable_if_t<std::is_same<T, OrFalse<T2>>::value>;

template<class T>
using enable_if_t_is_or_false_string = enable_if_t_is_or_false_t2<T, string>;
