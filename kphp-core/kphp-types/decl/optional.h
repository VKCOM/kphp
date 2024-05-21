// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "common/type_traits/is_constructible.h"
#include "common/type_traits/list_of_types.h"

#include "kphp-core/kphp-types/decl/declarations.h"
#include "kphp-core/functions/kphp-assert-core.h"

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
class OptionalBase {
public:
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

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

  bool is_null() const noexcept {
    return value_state_ == OptionalState::null_value;
  }

  bool is_false() const noexcept {
    return value_state_ == OptionalState::false_value;
  }

  OptionalState value_state() const noexcept {
    return value_state_;
  }

protected:
  template<class T1>
  OptionalBase(T1 &&value, OptionalState value_state) noexcept :
    value_(std::forward<T1>(value)),
    value_state_(value_state) {
  }

  explicit OptionalBase(OptionalState value_state) noexcept :
    value_state_(value_state) {
  }

  OptionalBase() = default;
  ~OptionalBase() = default;

  T value_{};
  OptionalState value_state_{OptionalState::null_value};
};

template<class T>
class Optional : public OptionalBase<T> {
private:
  template<class T1>
  using enable_if_allowed = std::enable_if_t<is_conversion_in_optional_allowed<T, T1>{}>;

  static_assert(!std::is_same<T, mixed>{}, "Usage Optional<mixed> is forbidden");
  static_assert(!is_optional<T>{}, "Usage Optional<Optional> is forbidden");

  using Base = OptionalBase<T>;

public:
  using InnerType = T;

  Optional() = default;

  Optional(bool value) noexcept :
    Optional(value, OptionalState::false_value) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(T1 &&value) noexcept :
    Optional(std::forward<T1>(value), OptionalState::has_value) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(const Optional<T1> &other) noexcept :
    Optional(other.val(), other.value_state()) {
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional(Optional<T1> &&other) noexcept :
    Optional(std::move(other.val()), other.value_state()) {
  }

  Optional &operator=(bool x) noexcept {
    return assign(x, OptionalState::false_value);
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(const Optional<T1> &other) noexcept {
    return assign(other.val(), other.value_state());
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(Optional<T1> &&other) noexcept {
    return assign(std::move(other.val()), other.value_state());
  }

  template<class T1, class = enable_if_allowed<T1>>
  Optional &operator=(T1 &&value) noexcept {
    return assign(std::forward<T1>(value), OptionalState::has_value);
  }

private:
  Optional(bool value, OptionalState value_state) noexcept :
    Base(value_state) {
    php_assert(!value);
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional(T1 &&value, OptionalState value_state) noexcept :
    Base(std::forward<T1>(value), value_state) {
  }

  Optional &assign(bool value, OptionalState value_state) noexcept {
    php_assert(!value);
    Base::value_ = T();
    Base::value_state_ = value_state;
    return *this;
  }

  template<class T1, class = vk::enable_if_constructible<T, T1>>
  Optional &assign(T1 &&value, OptionalState value_state) noexcept {
    Base::value_ = std::forward<T1>(value);
    Base::value_state_ = value_state;
    return *this;
  }
};

template<>
class Optional<bool> : public OptionalBase<bool> {
private:
  using Base = OptionalBase<bool>;

public:
  using InnerType = bool;

  Optional() = default;

  Optional(bool value) noexcept:
    Base(value, value ? OptionalState::has_value : OptionalState::false_value) {
  }

  Optional &operator=(bool value) noexcept {
    Base::value_ = value;
    Base::value_state_ = value ? OptionalState::has_value : OptionalState::false_value;
    return *this;
  }
};

template<class T, class ResT = void>
using enable_if_t_is_optional = std::enable_if_t<is_optional<std::decay_t<T>>::value, ResT>;

template<class T, class ResT = void>
using enable_if_t_is_not_optional = std::enable_if_t<!is_optional<std::decay_t<T>>{}, ResT>;

template<class T, class T2>
using enable_if_t_is_optional_t2 = std::enable_if_t<std::is_same<T, Optional<T2>>::value>;

template<class T>
using enable_if_t_is_optional_string = enable_if_t_is_optional_t2<T, string>;
