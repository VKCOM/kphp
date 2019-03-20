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


template<class T, class T1>
struct is_conversion_in_optional_allowed : std::is_constructible<T, T1> {
};

template<class T>
struct is_conversion_in_optional_allowed<T, bool> : std::true_type {
};

enum class OptionalState : uint8_t {
  has_value = 0,
  false_value = 1,
  null_value = 2
};

template<class T>
class Optional {
private:
  template<class T1>
  using enable_if_allowed = std::enable_if_t<is_conversion_in_optional_allowed<T, T1>{}>;

  static_assert(!std::is_same<T, var>{}, "Usage Optional<var> is forbidden");
  static_assert(!is_optional<T>{}, "Usage Optional<Optional> is forbidden");

public:
  using InnerType = T;

  Optional() = default;

  Optional(bool value) noexcept :
    Optional(value, OptionalState::false_value, std::is_same<T, bool>{}) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(T1 &&value) noexcept :
    Optional(std::forward<T1>(value), OptionalState::has_value, std::is_same<T, bool>{}) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(const Optional<T1> &other) noexcept :
    Optional(other.val(), other.value_state(), std::is_same<T, bool>{}) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(Optional<T1> &&other) noexcept :
    Optional(std::move(other.val()), other.value_state(), std::is_same<T, bool>{}) {
  }

  Optional &operator=(bool x) noexcept {
    return assign(x, OptionalState::false_value, std::is_same<T, bool>{});
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(const Optional<T1> &other) noexcept {
    return assign(other.val(), other.value_state(), std::is_same<T, bool>{});
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(Optional<T1> &&other) noexcept {
    return assign(std::move(other.val()), other.value_state(), std::is_same<T, bool>{});
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(T1 &&value) noexcept {
    return assign(std::forward<T1>(value), OptionalState::has_value, std::is_same<T, bool>{});
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

  OptionalState value_state() const noexcept {
    return value_state_;
  }

private:
  Optional(bool value, OptionalState, std::true_type /* bool_inside */) noexcept :
    value_(value),
    value_state_(value ? OptionalState::has_value : OptionalState::false_value) {
  }

  Optional(bool value, OptionalState value_state, std::false_type /* bool_inside */) noexcept :
    value_state_(value_state) {
    php_assert(!value);
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(T1 &&value, OptionalState value_state, std::false_type /* bool_inside */) noexcept :
    value_(std::forward<T1>(value)),
    value_state_(value_state) {
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(T1 && value, OptionalState value_state, std::true_type /* bool_inside */)  = delete;

  Optional &assign(bool value, OptionalState, std::true_type /* bool_inside */) noexcept {
    value_ = value;
    value_state_ = value ? OptionalState::has_value : OptionalState::false_value;
    return *this;
  }

  Optional &assign(bool value, OptionalState value_state, std::false_type /* bool_inside */) noexcept {
    php_assert(!value);
    value_ = T();
    value_state_ = value_state;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &assign(T1 &&value, OptionalState value_state, std::false_type /* bool_inside */) noexcept {
    value_ = std::forward<T1>(value);
    value_state_ = value_state;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &assign(T1 &&value, OptionalState value_state, std::true_type /* bool_inside */) = delete;


  T value_{};
  OptionalState value_state_{OptionalState::null_value};
};

template<class T, class ResT = void>
using enable_if_t_is_optional = std::enable_if_t<is_optional<std::decay_t<T>>::value, ResT>;

template<class T, class ResT = void>
using enable_if_t_is_not_optional = std::enable_if_t<!is_optional<std::decay_t<T>>{}, ResT>;

template<class T, class T2>
using enable_if_t_is_optional_t2 = std::enable_if_t<std::is_same<T, Optional<T2>>::value>;

template<class T>
using enable_if_t_is_optional_string = enable_if_t_is_optional_t2<T, string>;
