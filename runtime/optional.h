#pragma once

#include "common/type_traits/is_constructible.h"
#include "common/type_traits/list_of_types.h"

#include "runtime/php_assert.h"

template<class T>
class Optional;

template<class T>
struct is_optional : std::false_type {
};

template<class T>
struct is_optional<Optional<T>> : std::true_type {
};

enum class OptionalState : uint8_t {
  has_value = 0,
  false_value = 1,
  null_value = 2
};

template<class T>
class Optional {
public:
  static_assert(!vk::list_of_types<bool, var>::template in_list<T>(), "Usages Optional<bool> and Optional<var> are forbidden");
  static_assert(!is_optional<T>::value, "Usage Optional<Optional> is forbidden");

  using InnerType = T;

  Optional() = default;

  Optional(bool x) noexcept :
    value_state_(OptionalState::false_value) {
    php_assert(!x);
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(T1 &&x) noexcept :
    value_(std::forward<T1>(x)),
    value_state_(OptionalState::has_value) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(const Optional<T1> &other) noexcept :
    value_(other.val()),
    value_state_(other.value_status()) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(Optional<T1> &&other) noexcept :
    value_(std::move(other.val())),
    value_state_(other.value_status()) {
  }

  Optional &operator=(bool x) noexcept {
    php_assert(!x);
    value_ = T();
    value_state_ = OptionalState::false_value;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &operator=(const Optional<T1> &other) noexcept {
    value_ = T(other.val());
    value_state_ = other.value_status();
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &operator=(Optional<T1> &&other) noexcept {
    value_ = T(std::move(other.val()));
    value_state_ = other.value_status();
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &operator=(T1 &&x) noexcept {
    value_ = T(std::forward<T1>(x));
    value_state_ = OptionalState::has_value;
    return *this;
  }

  Optional &operator=(T &&x) noexcept {
    value_ = std::move(x);
    value_state_ = OptionalState::has_value;
    return *this;
  }

  Optional &operator=(const T &x) noexcept {
    value_ = x;
    value_state_ = OptionalState::has_value;
    return *this;
  }

  T &ref() noexcept {
    value_state_ = OptionalState::has_value;
    return value_;
  }

  T &val() noexcept {
    return value_;
  }

  const T &val() const noexcept {
    return value_;
  }

  bool has_value() const noexcept {
    return value_state_ == OptionalState::has_value;
  }

  OptionalState value_status() const noexcept {
    return value_state_;
  }

private:
  T value_{};
  OptionalState value_state_{OptionalState::false_value};
};

template<class T>
using enable_if_t_is_optional = std::enable_if_t<is_optional<std::decay_t<T>>::value>;

template<class T, class T2>
using enable_if_t_is_optional_t2 = std::enable_if_t<std::is_same<T, Optional<T2>>::value>;

template<class T>
using enable_if_t_is_optional_string = enable_if_t_is_optional_t2<T, string>;
