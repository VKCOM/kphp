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

enum class OrFalseOrNullState : uint8_t {
  has_value = 0,
  false_value = 1,
  null_value = 2
};

template<class T>
class OrFalse {
public:
  static_assert(!vk::list_of_types<bool, var>::template in_list<T>(), "Usages OrFalse<bool> and OrFalse<var> are forbidden");
  static_assert(!is_or_false<T>::value, "Usage OrFalse<OrFalse> is forbidden");

  using InnerType = T;

  OrFalse() = default;

  OrFalse(bool x) noexcept :
    value_state_(OrFalseOrNullState::false_value) {
    php_assert(!x);
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(T1 &&x) noexcept :
    value_(std::forward<T1>(x)),
    value_state_(OrFalseOrNullState::has_value) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(const OrFalse<T1> &other) noexcept :
    value_(other.val()),
    value_state_(other.value_status()) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse(OrFalse<T1> &&other) noexcept :
    value_(std::move(other.val())),
    value_state_(other.value_status()) {
  }

  OrFalse &operator=(bool x) noexcept {
    php_assert(!x);
    value_ = T();
    value_state_ = OrFalseOrNullState::false_value;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(const OrFalse<T1> &other) noexcept {
    value_ = T(other.val());
    value_state_ = other.value_status();
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(OrFalse<T1> &&other) noexcept {
    value_ = T(std::move(other.val()));
    value_state_ = other.value_status();
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  OrFalse &operator=(T1 &&x) noexcept {
    value_ = T(std::forward<T1>(x));
    value_state_ = OrFalseOrNullState::has_value;
    return *this;
  }

  OrFalse &operator=(T &&x) noexcept {
    value_ = std::move(x);
    value_state_ = OrFalseOrNullState::has_value;
    return *this;
  }

  OrFalse &operator=(const T &x) noexcept {
    value_ = x;
    value_state_ = OrFalseOrNullState::has_value;
    return *this;
  }

  T &ref() noexcept {
    value_state_ = OrFalseOrNullState::has_value;
    return value_;
  }

  T &val() noexcept {
    return value_;
  }

  const T &val() const noexcept {
    return value_;
  }

  bool has_value() const noexcept {
    return value_state_ == OrFalseOrNullState::has_value;
  }

  OrFalseOrNullState value_status() const noexcept {
    return value_state_;
  }

private:
  T value_{};
  OrFalseOrNullState value_state_{OrFalseOrNullState::false_value};
};

template<class T>
using enable_if_t_is_or_false = std::enable_if_t<is_or_false<std::decay_t<T>>::value>;

template<class T, class T2>
using enable_if_t_is_or_false_t2 = std::enable_if_t<std::is_same<T, OrFalse<T2>>::value>;

template<class T>
using enable_if_t_is_or_false_string = enable_if_t_is_or_false_t2<T, string>;
