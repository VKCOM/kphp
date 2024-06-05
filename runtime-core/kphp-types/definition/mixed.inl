// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/find.h"

#include "runtime-core/utils/migration-php8.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

static_assert(vk::all_of_equal(sizeof(__runtime_core::string<__runtime_core::DummyAllocator>), sizeof(double),
                               sizeof(__runtime_core::array<__runtime_core::mixed<__runtime_core::DummyAllocator>, __runtime_core::DummyAllocator>)),
              "sizeof of array<mixed>, string and double must be equal");

template<class T1, class T2>
inline int64_t spaceship(const T1 &lhs, const T2 &rhs);

namespace __runtime_core {
template<typename Allocator>
void mixed<Allocator>::copy_from(const mixed<Allocator> &other) {
  switch (other.get_type()) {
    case type::STRING:
      new (&as_string()) string(other.as_string());
      break;
    case type::ARRAY:
      new (&as_array()) array<mixed, Allocator>(other.as_array());
      break;
    default:
      storage_ = other.storage_;
  }
  type_ = other.get_type();
}

template<typename Allocator>
void mixed<Allocator>::copy_from(mixed<Allocator> &&other) {
  switch (other.get_type()) {
    case type::STRING:
      new (&as_string()) string(std::move(other.as_string()));
      break;
    case type::ARRAY:
      new (&as_array()) array<mixed, Allocator>(std::move(other.as_array()));
      break;
    default:
      storage_ = other.storage_;
  }
  type_ = other.get_type();
}

template<typename Allocator>
template<typename T>
void mixed<Allocator>::init_from(T &&v) {
  auto type_and_value_ref = get_type_and_value_ptr(v);
  type_ = type_and_value_ref.first;
  auto *value_ptr = type_and_value_ref.second;
  using ValueType = std::decay_t<decltype(*value_ptr)>;
  new (value_ptr) ValueType(std::forward<T>(v));
}

template<typename Allocator>
template<typename T>
mixed<Allocator> &mixed<Allocator>::assign_from(T &&v) {
  auto type_and_value_ref = get_type_and_value_ptr(v);
  if (get_type() == type_and_value_ref.first) {
    *type_and_value_ref.second = std::forward<T>(v);
  } else {
    destroy();
    init_from(std::forward<T>(v));
  }
  return *this;
}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator>::mixed(T &&v) noexcept {
  init_from(std::forward<T>(v));
}

template<typename Allocator>
mixed<Allocator>::mixed(const Unknown &u __attribute__((unused))) noexcept {
  php_assert("Unknown used!!!" && 0);
}

template<typename Allocator>
mixed<Allocator>::mixed(const char *s, typename string<Allocator>::size_type len) noexcept
  : mixed(string<Allocator>{s, len}) {}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator>::mixed(const Optional<T> &v) noexcept {
  auto init_from_lambda = [this](const auto &v) { this->init_from(v); };
  call_fun_on_optional_value(init_from_lambda, v);
}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator>::mixed(Optional<T> &&v) noexcept {
  auto init_from_lambda = [this](auto &&v) { this->init_from(std::move(v)); };
  call_fun_on_optional_value(init_from_lambda, std::move(v));
}

template<typename Allocator>
mixed<Allocator>::mixed(const mixed &v) noexcept {
  copy_from(v);
}

template<typename Allocator>
mixed<Allocator>::mixed(mixed &&v) noexcept {
  copy_from(std::move(v));
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator=(const mixed &other) noexcept {
  if (this != &other) {
    destroy();
    copy_from(other);
  }
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator=(mixed &&other) noexcept {
  if (this != &other) {
    destroy();
    copy_from(std::move(other));
  }
  return *this;
}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator> &mixed<Allocator>::operator=(T &&v) noexcept {
  return assign_from(std::forward<T>(v));
}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator> &mixed<Allocator>::operator=(const Optional<T> &v) noexcept {
  auto assign_from_lambda = [this](const auto &v) -> mixed & { return this->assign_from(v); };
  return call_fun_on_optional_value(assign_from_lambda, v);
}

template<typename Allocator>
template<typename T, typename>
mixed<Allocator> &mixed<Allocator>::operator=(Optional<T> &&v) noexcept {
  auto assign_from_lambda = [this](auto &&v) -> mixed & { return this->assign_from(std::move(v)); };
  return call_fun_on_optional_value(assign_from_lambda, std::move(v));
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::assign(const char *other, typename string<Allocator>::size_type len) {
  if (get_type() == type::STRING) {
    as_string().assign(other, len);
  } else {
    destroy();
    type_ = type::STRING;
    new (&as_string()) string<Allocator>(other, len);
  }
  return *this;
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::operator-() const {
  mixed arg1 = to_numeric();

  if (arg1.get_type() == type::INTEGER) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::operator+() const {
  return to_numeric();
}

template<typename Allocator>
int64_t mixed<Allocator>::operator~() const {
  return ~to_int();
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator+=(const mixed &other) {
  if (likely(get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
    as_int() += other.as_int();
    return *this;
  }

  if (unlikely(get_type() == type::ARRAY || other.get_type() == type::ARRAY)) {
    if (get_type() == type::ARRAY && other.get_type() == type::ARRAY) {
      as_array() += other.as_array();
    } else {
      php_warning("Unsupported operand types for operator += (%s and %s)", get_type_c_str(), other.get_type_c_str());
    }
    return *this;
  }

  convert_to_numeric();
  const mixed arg2 = other.to_numeric();

  if (get_type() == type::INTEGER) {
    if (arg2.get_type() == type::INTEGER) {
      as_int() += arg2.as_int();
    } else {
      type_ = type::FLOAT;
      as_double() = static_cast<double>(as_int()) + arg2.as_double();
    }
  } else {
    if (arg2.get_type() == type::INTEGER) {
      as_double() += static_cast<double>(arg2.as_int());
    } else {
      as_double() += arg2.as_double();
    }
  }

  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator-=(const mixed &other) {
  if (likely(get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
    as_int() -= other.as_int();
    return *this;
  }

  convert_to_numeric();
  const mixed arg2 = other.to_numeric();

  if (get_type() == type::INTEGER) {
    if (arg2.get_type() == type::INTEGER) {
      as_int() -= arg2.as_int();
    } else {
      type_ = type::FLOAT;
      as_double() = static_cast<double>(as_int()) - arg2.as_double();
    }
  } else {
    if (arg2.get_type() == type::INTEGER) {
      as_double() -= static_cast<double>(arg2.as_int());
    } else {
      as_double() -= arg2.as_double();
    }
  }

  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator*=(const mixed &other) {
  if (likely(get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
    as_int() *= other.as_int();
    return *this;
  }

  convert_to_numeric();
  const mixed arg2 = other.to_numeric();

  if (get_type() == type::INTEGER) {
    if (arg2.get_type() == type::INTEGER) {
      as_int() *= arg2.as_int();
    } else {
      type_ = type::FLOAT;
      as_double() = static_cast<double>(as_int()) * arg2.as_double();
    }
  } else {
    if (arg2.get_type() == type::INTEGER) {
      as_double() *= static_cast<double>(arg2.as_int());
    } else {
      as_double() *= arg2.as_double();
    }
  }

  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator/=(const mixed &other) {
  if (likely(get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
    if (as_int() % other.as_int() == 0) {
      as_int() /= other.as_int();
    } else {
      type_ = type::FLOAT;
      as_double() = static_cast<double>(as_int()) / static_cast<double>(other.as_int());
    }
    return *this;
  }

  convert_to_numeric();
  const mixed arg2 = other.to_numeric();

  if (arg2.get_type() == type::INTEGER) {
    if (arg2.as_int() == 0) {
      php_warning("Integer division by zero");
      type_ = type::BOOLEAN;
      as_bool() = false;
      return *this;
    }

    if (get_type() == type::INTEGER) {
      if (as_int() % arg2.as_int() == 0) {
        as_int() /= arg2.as_int();
      } else {
        type_ = type::FLOAT;
        as_double() = static_cast<double>(as_int()) / static_cast<double>(other.as_int());
      }
    } else {
      as_double() /= static_cast<double>(arg2.as_int());
    }
  } else {
    if (arg2.as_double() == 0) {
      php_warning("Float division by zero");
      type_ = type::BOOLEAN;
      as_bool() = false;
      return *this;
    }

    if (get_type() == type::INTEGER) {
      type_ = type::FLOAT;
      as_double() = static_cast<double>(as_int()) / arg2.as_double();
    } else {
      as_double() /= arg2.as_double();
    }
  }

  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator%=(const mixed &other) {
  int64_t div = other.to_int();
  if (div == 0) {
    php_warning("Modulo by zero");
    *this = false;
    return *this;
  }
  convert_to_int();
  as_int() %= div;

  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator&=(const mixed &other) {
  convert_to_int();
  as_int() &= other.to_int();
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator|=(const mixed &other) {
  convert_to_int();
  as_int() |= other.to_int();
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator^=(const mixed &other) {
  convert_to_int();
  as_int() ^= other.to_int();
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator<<=(const mixed &other) {
  convert_to_int();
  as_int() <<= other.to_int();
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator>>=(const mixed &other) {
  convert_to_int();
  as_int() >>= other.to_int();
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator++() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::INTEGER;
      as_int() = 1;
      return *this;
    case type::BOOLEAN:
      php_warning("Can't apply operator ++ to boolean");
      return *this;
    case type::INTEGER:
      ++as_int();
      return *this;
    case type::FLOAT:
      as_double() += 1;
      return *this;
    case type::STRING:
      *this = as_string().to_numeric();
      return ++(*this);
    case type::ARRAY:
      php_warning("Can't apply operator ++ to array");
      return *this;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::operator++(int) {
  switch (get_type()) {
    case type::NUL:
      type_ = type::INTEGER;
      as_int() = 1;
      return mixed();
    case type::BOOLEAN:
      php_warning("Can't apply operator ++ to boolean");
      return as_bool();
    case type::INTEGER: {
      mixed res(as_int());
      ++as_int();
      return res;
    }
    case type::FLOAT: {
      mixed res(as_double());
      as_double() += 1;
      return res;
    }
    case type::STRING: {
      mixed res(as_string());
      *this = as_string().to_numeric();
      (*this)++;
      return res;
    }
    case type::ARRAY:
      php_warning("Can't apply operator ++ to array");
      return as_array();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator--() {
  if (likely(get_type() == type::INTEGER)) {
    --as_int();
    return *this;
  }

  switch (get_type()) {
    case type::NUL:
      php_warning("Can't apply operator -- to null");
      return *this;
    case type::BOOLEAN:
      php_warning("Can't apply operator -- to boolean");
      return *this;
    case type::INTEGER:
      --as_int();
      return *this;
    case type::FLOAT:
      as_double() -= 1;
      return *this;
    case type::STRING:
      *this = as_string().to_numeric();
      return --(*this);
    case type::ARRAY:
      php_warning("Can't apply operator -- to array");
      return *this;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::operator--(int) {
  if (likely(get_type() == type::INTEGER)) {
    mixed res(as_int());
    --as_int();
    return res;
  }

  switch (get_type()) {
    case type::NUL:
      php_warning("Can't apply operator -- to null");
      return mixed();
    case type::BOOLEAN:
      php_warning("Can't apply operator -- to boolean");
      return as_bool();
    case type::INTEGER: {
      mixed res(as_int());
      --as_int();
      return res;
    }
    case type::FLOAT: {
      mixed res(as_double());
      as_double() -= 1;
      return res;
    }
    case type::STRING: {
      mixed res(as_string());
      *this = as_string().to_numeric();
      (*this)--;
      return res;
    }
    case type::ARRAY:
      php_warning("Can't apply operator -- to array");
      return as_array();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
bool mixed<Allocator>::operator!() const {
  return !to_bool();
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::append(const string<Allocator> &v) {
  if (unlikely(get_type() != type::STRING)) {
    convert_to_string();
  }
  as_string().append(v);
  return *this;
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::append(tmp_string<Allocator> v) {
  if (unlikely(get_type() != type::STRING)) {
    convert_to_string();
  }
  as_string().append(v.data, v.size);
  return *this;
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::to_numeric() const {
  switch (get_type()) {
    case type::NUL:
      return 0;
    case type::BOOLEAN:
      return (as_bool() ? 1 : 0);
    case type::INTEGER:
      return as_int();
    case type::FLOAT:
      return as_double();
    case type::STRING:
      return as_string().to_numeric();
    case type::ARRAY:
      php_warning("Wrong conversion from array to number");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
bool mixed<Allocator>::to_bool() const {
  switch (get_type()) {
    case type::NUL:
      return false;
    case type::BOOLEAN:
      return as_bool();
    case type::INTEGER:
      return (bool)as_int();
    case type::FLOAT:
      return (bool)as_double();
    case type::STRING:
      return as_string().to_bool();
    case type::ARRAY:
      return !as_array().empty();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
int64_t mixed<Allocator>::to_int() const {
  switch (get_type()) {
    case type::NUL:
      return 0;
    case type::BOOLEAN:
      return static_cast<int64_t>(as_bool());
    case type::INTEGER:
      return as_int();
    case type::FLOAT:
      return static_cast<int64_t>(as_double());
    case type::STRING:
      return as_string().to_int();
    case type::ARRAY:
      php_warning("Wrong conversion from array to int");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
double mixed<Allocator>::to_float() const {
  switch (get_type()) {
    case type::NUL:
      return 0.0;
    case type::BOOLEAN:
      return (as_bool() ? 1.0 : 0.0);
    case type::INTEGER:
      return (double)as_int();
    case type::FLOAT:
      return as_double();
    case type::STRING:
      return as_string().to_float();
    case type::ARRAY:
      php_warning("Wrong conversion from array to float");
      return as_array().to_float();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const string<Allocator> mixed<Allocator>::to_string() const {
  switch (get_type()) {
    case type::NUL:
      return string<Allocator>();
    case type::BOOLEAN:
      return (as_bool() ? string<Allocator>("1", 1) : string<Allocator>());
    case type::INTEGER:
      return string<Allocator>(as_int());
    case type::FLOAT:
      return string<Allocator>(as_double());
    case type::STRING:
      return as_string();
    case type::ARRAY:
      php_warning("Conversion from array to string");
      return string<Allocator>("Array", 5);
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const array<mixed<Allocator>, Allocator> mixed<Allocator>::to_array() const {
  switch (get_type()) {
    case type::NUL:
      return array<mixed, Allocator>();
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
    case type::STRING: {
      array<mixed, Allocator> res(array_size(1, true));
      res.push_back(*this);
      return res;
    }
    case type::ARRAY:
      return as_array();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
bool &mixed<Allocator>::as_bool() {
  return *reinterpret_cast<bool *>(&storage_);
}

template<typename Allocator>
const bool &mixed<Allocator>::as_bool() const {
  return *reinterpret_cast<const bool *>(&storage_);
}

template<typename Allocator>
int64_t &mixed<Allocator>::as_int() {
  return *reinterpret_cast<int64_t *>(&storage_);
}

template<typename Allocator>
const int64_t &mixed<Allocator>::as_int() const {
  return *reinterpret_cast<const int64_t *>(&storage_);
}

template<typename Allocator>
double &mixed<Allocator>::as_double() {
  return *reinterpret_cast<double *>(&storage_);
}

template<typename Allocator>
const double &mixed<Allocator>::as_double() const {
  return *reinterpret_cast<const double *>(&storage_);
}

template<typename Allocator>
string<Allocator> &mixed<Allocator>::as_string() {
  return *reinterpret_cast<string<Allocator> *>(&storage_);
}

template<typename Allocator>
const string<Allocator> &mixed<Allocator>::as_string() const {
  return *reinterpret_cast<const string<Allocator> *>(&storage_);
}

template<typename Allocator>
array<mixed<Allocator>, Allocator> &mixed<Allocator>::as_array() {
  return *reinterpret_cast<array<mixed, Allocator> *>(&storage_);
}

template<typename Allocator>
const array<mixed<Allocator>, Allocator> &mixed<Allocator>::as_array() const {
  return *reinterpret_cast<const array<mixed, Allocator> *>(&storage_);
}

template<typename Allocator>
int64_t mixed<Allocator>::safe_to_int() const {
  switch (get_type()) {
    case type::NUL:
      return 0;
    case type::BOOLEAN:
      return static_cast<int64_t>(as_bool());
    case type::INTEGER:
      return as_int();
    case type::FLOAT: {
      constexpr auto max_int = static_cast<double>(static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1);
      if (fabs(as_double()) > max_int) {
        php_warning("Wrong conversion from double %.6lf to int", as_double());
      }
      return static_cast<int64_t>(as_double());
    }
    case type::STRING:
      return as_string().safe_to_int();
    case type::ARRAY:
      php_warning("Wrong conversion from array to int");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::convert_to_numeric() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::INTEGER;
      as_int() = 0;
      return;
    case type::BOOLEAN:
      type_ = type::INTEGER;
      as_int() = as_bool();
      return;
    case type::INTEGER:
    case type::FLOAT:
      return;
    case type::STRING:
      *this = as_string().to_numeric();
      return;
    case type::ARRAY: {
      php_warning("Wrong conversion from array to number");
      const int64_t int_val = as_array().to_int();
      as_array().~array<mixed, Allocator>();
      type_ = type::INTEGER;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::convert_to_bool() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::BOOLEAN;
      as_bool() = 0;
      return;
    case type::BOOLEAN:
      return;
    case type::INTEGER:
      type_ = type::BOOLEAN;
      as_bool() = (bool)as_int();
      return;
    case type::FLOAT:
      type_ = type::BOOLEAN;
      as_bool() = (bool)as_double();
      return;
    case type::STRING: {
      const bool bool_val = as_string().to_bool();
      as_string().~string();
      type_ = type::BOOLEAN;
      as_bool() = bool_val;
      return;
    }
    case type::ARRAY: {
      const bool bool_val = as_array().to_bool();
      as_array().~array<mixed, Allocator>();
      type_ = type::BOOLEAN;
      as_bool() = bool_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::convert_to_int() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::INTEGER;
      as_int() = 0;
      return;
    case type::BOOLEAN:
      type_ = type::INTEGER;
      as_int() = as_bool();
      return;
    case type::INTEGER:
      return;
    case type::FLOAT:
      type_ = type::INTEGER;
      as_int() = static_cast<int64_t>(as_double());
      return;
    case type::STRING: {
      const int64_t int_val = as_string().to_int();
      as_string().~string();
      type_ = type::INTEGER;
      as_int() = int_val;
      return;
    }
    case type::ARRAY: {
      php_warning("Wrong conversion from array to int");
      const int64_t int_val = as_array().to_int();
      as_array().~array<mixed, Allocator>();
      type_ = type::INTEGER;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::convert_to_float() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::FLOAT;
      as_double() = 0.0;
      return;
    case type::BOOLEAN:
      type_ = type::FLOAT;
      as_double() = as_bool();
      return;
    case type::INTEGER:
      type_ = type::FLOAT;
      as_double() = (double)as_int();
      return;
    case type::FLOAT:
      return;
    case type::STRING: {
      const double float_val = as_string().to_float();
      as_string().~string();
      type_ = type::FLOAT;
      as_double() = float_val;
      return;
    }
    case type::ARRAY: {
      php_warning("Wrong conversion from array to float");
      const double float_val = as_array().to_float();
      as_array().~array<mixed, Allocator>();
      type_ = type::FLOAT;
      as_double() = float_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::convert_to_string() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::STRING;
      new (&as_string()) string<Allocator>();
      return;
    case type::BOOLEAN:
      type_ = type::STRING;
      if (as_bool()) {
        new (&as_string()) string<Allocator>("1", 1);
      } else {
        new (&as_string()) string<Allocator>();
      }
      return;
    case type::INTEGER:
      type_ = type::STRING;
      new (&as_string()) string<Allocator>(as_int());
      return;
    case type::FLOAT:
      type_ = type::STRING;
      new (&as_string()) string<Allocator>(as_double());
      return;
    case type::STRING:
      return;
    case type::ARRAY:
      php_warning("Converting from array to string");
      as_array().~array<mixed, Allocator>();
      type_ = type::STRING;
      new (&as_string()) string<Allocator>("Array", 5);
      return;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const bool &mixed<Allocator>::as_bool(const char *function) const {
  switch (get_type()) {
    case type::BOOLEAN:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_c_str());
      return empty_value<bool>();
  }
}

template<typename Allocator>
const int64_t &mixed<Allocator>::as_int(const char *function) const {
  switch (get_type()) {
    case type::INTEGER:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_c_str());
      return empty_value<int64_t>();
  }
}

template<typename Allocator>
const double &mixed<Allocator>::as_float(const char *function) const {
  switch (get_type()) {
    case type::FLOAT:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_c_str());
      return empty_value<double>();
  }
}

template<typename Allocator>
const string<Allocator> &mixed<Allocator>::as_string(const char *function) const {
  switch (get_type()) {
    case type::STRING:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_c_str());
      return empty_value<string<Allocator>>();
  }
}

template<typename Allocator>
const array<mixed<Allocator>, Allocator> &mixed<Allocator>::as_array(const char *function) const {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      return empty_value<array<mixed, Allocator>>();
  }
}

template<typename Allocator>
bool &mixed<Allocator>::as_bool(const char *function) {
  switch (get_type()) {
    case type::NUL:
      convert_to_bool();
    case type::BOOLEAN:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_c_str());
      return empty_value<bool>();
  }
}

template<typename Allocator>
int64_t &mixed<Allocator>::as_int(const char *function) {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::FLOAT:
    case type::STRING:
      convert_to_int();
    case type::INTEGER:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_c_str());
      return empty_value<int64_t>();
  }
}

template<typename Allocator>
double &mixed<Allocator>::as_float(const char *function) {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::INTEGER:
    case type::STRING:
      convert_to_float();
    case type::FLOAT:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_c_str());
      return empty_value<double>();
  }
}

template<typename Allocator>
string<Allocator> &mixed<Allocator>::as_string(const char *function) {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
      convert_to_string();
    case type::STRING:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_c_str());
      return empty_value<string<Allocator>>();
  }
}

template<typename Allocator>
array<mixed<Allocator>, Allocator> &mixed<Allocator>::as_array(const char *function) {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      return empty_value<array<mixed, Allocator>>();
  }
}

template<typename Allocator>
bool mixed<Allocator>::is_numeric() const {
  switch (get_type()) {
    case type::INTEGER:
    case type::FLOAT:
      return true;
    case type::STRING:
      return as_string().is_numeric();
    default:
      return false;
  }
}

template<typename Allocator>
bool mixed<Allocator>::is_scalar() const {
  return get_type() != type::NUL && get_type() != type::ARRAY;
}

template<typename Allocator>
typename mixed<Allocator>::type mixed<Allocator>::get_type() const {
  return type_;
}

template<typename Allocator>
bool mixed<Allocator>::is_null() const {
  return get_type() == type::NUL;
}

template<typename Allocator>
bool mixed<Allocator>::is_bool() const {
  return get_type() == type::BOOLEAN;
}

template<typename Allocator>
bool mixed<Allocator>::is_int() const {
  return get_type() == type::INTEGER;
}

template<typename Allocator>
bool mixed<Allocator>::is_float() const {
  return get_type() == type::FLOAT;
}

template<typename Allocator>
bool mixed<Allocator>::is_string() const {
  return get_type() == type::STRING;
}

template<typename Allocator>
bool mixed<Allocator>::is_array() const {
  return get_type() == type::ARRAY;
}

template<typename Allocator>
inline const char *mixed<Allocator>::get_type_c_str() const {
  switch (get_type()) {
    case type::NUL:
      return "NULL";
    case type::BOOLEAN:
      return "boolean";
    case type::INTEGER:
      return "integer";
    case type::FLOAT:
      return "double";
    case type::STRING:
      return "string";
    case type::ARRAY:
      return "array";
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
inline const string<Allocator> mixed<Allocator>::get_type_str() const {
  return string<Allocator>(get_type_c_str());
}

template<typename Allocator>
bool mixed<Allocator>::empty() const {
  return !to_bool();
}

template<typename Allocator>
int64_t mixed<Allocator>::count() const {
  switch (get_type()) {
    case type::NUL:
      php_warning("count(): Parameter is null, but an array expected");
      return 0;
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
    case type::STRING:
      php_warning("count(): Parameter is %s, but an array expected", get_type_c_str());
      return 1;
    case type::ARRAY:
      return as_array().count();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
int64_t mixed<Allocator>::compare(const mixed<Allocator> &rhs) const {
  if (unlikely(is_string())) {
    if (likely(rhs.is_string())) {
      return compare_strings_php_order(as_string(), rhs.as_string());
    } else if (unlikely(rhs.is_null())) {
      return as_string().empty() ? 0 : 1;
    }
  } else if (unlikely(rhs.is_string())) {
    if (unlikely(is_null())) {
      return rhs.as_string().empty() ? 0 : -1;
    }
  }
  if (is_bool() || rhs.is_bool() || is_null() || rhs.is_null()) {
    return three_way_comparison(to_bool(), rhs.to_bool());
  }

  if (unlikely(is_array() || rhs.is_array())) {
    if (likely(is_array() && rhs.is_array())) {
      return spaceship(as_array(), rhs.as_array());
    }

    php_warning("Unsupported operand types for operator < or <= (%s and %s)", get_type_c_str(), rhs.get_type_c_str());
    return is_array() ? 1 : -1;
  }

  return three_way_comparison(to_float(), rhs.to_float());
}

template<typename Allocator>
void mixed<Allocator>::swap(mixed<Allocator> &other) {
  ::swap(type_, other.type_);
  ::swap(as_double(), other.as_double());
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](int64_t int_key) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      php_warning("Writing to string by offset is't supported");
      return empty_value<mixed>();
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
      return empty_value<mixed>();
    }
  }
  return as_array()[int_key];
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](const string<Allocator> &string_key) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      php_warning("Writing to string by offset is't supported");
      return empty_value<mixed>();
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
      return empty_value<mixed>();
    }
  }

  return as_array()[string_key];
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](tmp_string<Allocator> string_key) {
  if (get_type() == type::ARRAY) {
    return as_array()[string_key];
  }
  return (*this)[materialize_tmp_string(string_key)];
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](const mixed<Allocator> &v) {
  switch (v.get_type()) {
    case type::NUL:
      return (*this)[string<Allocator>()];
    case type::BOOLEAN:
      return (*this)[v.as_bool()];
    case type::INTEGER:
      return (*this)[v.as_int()];
    case type::FLOAT:
      return (*this)[static_cast<int64_t>(v.as_double())];
    case type::STRING:
      return (*this)[v.as_string()];
    case type::ARRAY:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return (*this)[v.as_array().to_int()];
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](double double_key) {
  return (*this)[static_cast<int64_t>(double_key)];
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](const typename array<mixed, Allocator>::const_iterator &it) {
  return as_array()[it];
}

template<typename Allocator>
mixed<Allocator> &mixed<Allocator>::operator[](const typename array<mixed, Allocator>::iterator &it) {
  return as_array()[it];
}

template<typename Allocator>
void mixed<Allocator>::set_value(int64_t int_key, const mixed &v) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      auto rhs_string = v.to_string();
      if (rhs_string.empty()) {
        php_warning("Cannot assign an empty string to a string offset, index = %" PRIi64, int_key);
        return;
      }

      const char c = rhs_string[0];

      if (int_key >= 0) {
        const typename string<Allocator>::size_type l = as_string().size();
        if (int_key >= l) {
          as_string().append(string<Allocator>::unsafe_cast_to_size_type(int_key + 1 - l), ' ');
        } else {
          as_string().make_not_shared();
        }

        as_string()[static_cast<typename string<Allocator>::size_type>(int_key)] = c;
      } else {
        php_warning("%" PRIi64 " is illegal offset for string", int_key);
      }
      return;
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
      return;
    }
  }
  return as_array().set_value(int_key, v);
}

template<typename Allocator>
void mixed<Allocator>::set_value(const string<Allocator> &string_key, const mixed &v) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      int64_t int_val = 0;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        int_val = string_key.to_int();
      }
      if (int_val < 0) {
        return;
      }

      char c = (v.to_string())[0];

      const typename string<Allocator>::size_type l = as_string().size();
      if (int_val >= l) {
        as_string().append(string<Allocator>::unsafe_cast_to_size_type(int_val + 1 - l), ' ');
      } else {
        as_string().make_not_shared();
      }

      as_string()[static_cast<typename string<Allocator>::size_type>(int_val)] = c;
      return;
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
      return;
    }
  }

  return as_array().set_value(string_key, v);
}

template<typename Allocator>
void mixed<Allocator>::set_value(const string<Allocator> &string_key, const mixed &v, int64_t precomuted_hash) {
  return get_type() == type::ARRAY ? as_array().set_value(string_key, v, precomuted_hash) : set_value(string_key, v);
}

template<typename Allocator>
void mixed<Allocator>::set_value(tmp_string<Allocator> string_key, const mixed &v) {
  // TODO: as with arrays, avoid eager tmp_string->string conversion
  set_value(materialize_tmp_string(string_key), v);
}

template<typename Allocator>
void mixed<Allocator>::set_value(const mixed &v, const mixed &value) {
  switch (v.get_type()) {
    case type::NUL:
      return set_value(string<Allocator>(), value);
    case type::BOOLEAN:
      return set_value(static_cast<int64_t>(v.as_bool()), value);
    case type::INTEGER:
      return set_value(v.as_int(), value);
    case type::FLOAT:
      return set_value(static_cast<int64_t>(v.as_double()), value);
    case type::STRING:
      return set_value(v.as_string(), value);
    case type::ARRAY:
      php_warning("Illegal offset type array");
      return;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::set_value(double double_key, const mixed &value) {
  set_value(static_cast<int64_t>(double_key), value);
}

template<typename Allocator>
void mixed<Allocator>::set_value(const typename array<mixed, Allocator>::const_iterator &it) {
  return as_array().set_value(it);
}

template<typename Allocator>
void mixed<Allocator>::set_value(const typename array<mixed, Allocator>::iterator &it) {
  return as_array().set_value(it);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(int64_t int_key) const {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      if (int_key < 0 || int_key >= as_string().size()) {
        return string<Allocator>();
      }
      return string<Allocator>(1, as_string()[static_cast<typename string<Allocator>::size_type>(int_key)]);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
    }
    return mixed();
  }

  return as_array().get_value(int_key);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(const string<Allocator> &string_key) const {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      int64_t int_val = 0;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        int_val = string_key.to_int();
      }
      if (int_val < 0 || int_val >= as_string().size()) {
        return string<Allocator>();
      }
      return string<Allocator>(1, as_string()[static_cast<typename string<Allocator>::size_type>(int_val)]);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
    }
    return mixed();
  }

  return as_array().get_value(string_key);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(const string<Allocator> &string_key, int64_t precomuted_hash) const {
  return get_type() == type::ARRAY ? as_array().get_value(string_key, precomuted_hash) : get_value(string_key);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(tmp_string<Allocator> string_key) const {
  if (get_type() == type::ARRAY) {
    // fast path: arrays can handle a tmp_string lookup efficiently
    return as_array().get_value(string_key);
  }
  // TODO: make other lookups efficient too (like string from numeric tmp_string)?
  return get_value(materialize_tmp_string(string_key));
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(const mixed &v) const {
  switch (v.get_type()) {
    case type::NUL:
      return get_value(string<Allocator>());
    case type::BOOLEAN:
      return get_value(static_cast<int64_t>(v.as_bool()));
    case type::INTEGER:
      return get_value(v.as_int());
    case type::FLOAT:
      return get_value(static_cast<int64_t>(v.as_double()));
    case type::STRING:
      return get_value(v.as_string());
    case type::ARRAY:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return mixed();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(double double_key) const {
  return get_value(static_cast<int64_t>(double_key));
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(const typename array<mixed, Allocator>::const_iterator &it) const {
  return as_array().get_value(it);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::get_value(const typename array<mixed, Allocator>::iterator &it) const {
  return as_array().get_value(it);
}

template<typename Allocator>
void mixed<Allocator>::push_back(const mixed &v) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      return;
    }
  }

  return as_array().push_back(v);
}

template<typename Allocator>
const mixed<Allocator> mixed<Allocator>::push_back_return(const mixed &v) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new (&as_array()) array<mixed, Allocator>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      return empty_value<mixed>();
    }
  }

  return as_array().push_back_return(v);
}

template<typename Allocator>
bool mixed<Allocator>::isset(int64_t int_key) const {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      int_key = as_string().get_correct_index(int_key);
      return as_string().isset(int_key);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in isset", get_type_c_str());
    }
    return false;
  }

  return as_array().isset(int_key);
}

template<typename Allocator>
template<class... MaybeHash>
bool mixed<Allocator>::isset(const string<Allocator> &string_key, MaybeHash... maybe_hash) const {
  if (unlikely(get_type() != type::ARRAY)) {
    int64_t int_key{std::numeric_limits<int64_t>::max()};
    if (get_type() == type::STRING) {
      if (!string_key.try_to_int(&int_key)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        return false;
      }
    }

    return isset(int_key);
  }

  return as_array().isset(string_key, maybe_hash...);
}

template<typename Allocator>
bool mixed<Allocator>::isset(const mixed<Allocator> &v) const {
  switch (v.get_type()) {
    case type::NUL:
      return isset(string<Allocator>());
    case type::BOOLEAN:
      return isset(static_cast<int64_t>(v.as_bool()));
    case type::INTEGER:
      return isset(v.as_int());
    case type::FLOAT:
      return isset(static_cast<int64_t>(v.as_double()));
    case type::STRING:
      return isset(v.as_string());
    case type::ARRAY:
      php_warning("Illegal offset type array");
      return false;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
bool mixed<Allocator>::isset(double double_key) const {
  return isset(static_cast<int64_t>(double_key));
}

template<typename Allocator>
void mixed<Allocator>::unset(int64_t int_key) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  as_array().unset(int_key);
}

template<typename Allocator>
template<class... MaybeHash>
void mixed<Allocator>::unset(const string<Allocator> &string_key, MaybeHash... maybe_hash) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  as_array().unset(string_key, maybe_hash...);
}

template<typename Allocator>
void mixed<Allocator>::unset(const mixed &v) {
  if (unlikely(get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  switch (v.get_type()) {
    case type::NUL:
      as_array().unset(string<Allocator>());
      break;
    case type::BOOLEAN:
      as_array().unset(static_cast<int64_t>(v.as_bool()));
      break;
    case type::INTEGER:
      as_array().unset(v.as_int());
      break;
    case type::FLOAT:
      as_array().unset(static_cast<int64_t>(v.as_double()));
      break;
    case type::STRING:
      as_array().unset(v.as_string());
      break;
    case type::ARRAY:
      php_warning("Illegal offset type array");
      break;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::unset(double double_key) {
  unset(static_cast<int64_t>(double_key));
}

template<typename Allocator>
typename array<mixed<Allocator>, Allocator>::const_iterator mixed<Allocator>::begin() const {
  if (likely(get_type() == type::ARRAY)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_c_str(), to_string().c_str());
  return typename array<mixed, Allocator>::const_iterator();
}

template<typename Allocator>
typename array<mixed<Allocator>, Allocator>::const_iterator mixed<Allocator>::end() const {
  if (likely(get_type() == type::ARRAY)) {
    return as_array().end();
  }
  return typename array<mixed, Allocator>::const_iterator();
}

template<typename Allocator>
typename array<mixed<Allocator>, Allocator>::iterator mixed<Allocator>::begin() {
  if (likely(get_type() == type::ARRAY)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_c_str(), to_string().c_str());
  return array<mixed, Allocator>::iterator();
}

template<typename Allocator>
typename array<mixed<Allocator>, Allocator>::iterator mixed<Allocator>::end() {
  if (likely(get_type() == type::ARRAY)) {
    return as_array().end();
  }
  return array<mixed, Allocator>::iterator();
}

template<typename Allocator>
int64_t mixed<Allocator>::get_reference_counter() const {
  switch (get_type()) {
    case type::NUL:
      return -1;
    case type::BOOLEAN:
      return -2;
    case type::INTEGER:
      return -3;
    case type::FLOAT:
      return -4;
    case type::STRING:
      return as_string().get_reference_counter();
    case type::ARRAY:
      return as_array().get_reference_counter();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
      return;
    case type::STRING:
      return as_string().set_reference_counter_to(ref_cnt_value);
    case type::ARRAY:
      return as_array().set_reference_counter_to(ref_cnt_value);
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
inline bool mixed<Allocator>::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
      return false;
    case type::STRING:
      return as_string().is_reference_counter(ref_cnt_value);
    case type::ARRAY:
      return as_array().is_reference_counter(ref_cnt_value);
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
inline void mixed<Allocator>::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().force_destroy(expected_ref_cnt);
      break;
    case type::ARRAY:
      as_array().force_destroy(expected_ref_cnt);
      break;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
size_t mixed<Allocator>::estimate_memory_usage() const {
  switch (get_type()) {
    case type::NUL:
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
      return 0;
    case type::STRING:
      return as_string().estimate_memory_usage();
    case type::ARRAY:
      return as_array().estimate_memory_usage();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void mixed<Allocator>::reset_empty_values() noexcept {
  empty_value<bool>();
  empty_value<int64_t>();
  empty_value<double>();
  empty_value<string<Allocator>>();
  empty_value<mixed<Allocator>>();
  empty_value<array<mixed<Allocator>, Allocator>>();
}

template<typename Allocator>
template<typename T>
T &mixed<Allocator>::empty_value() noexcept {
  static_assert(vk::is_type_in_list<T, bool, int64_t, double, string<Allocator>, mixed, array<mixed, Allocator>>{}, "unsupported type");
  static T value;
  value = T{};
  return value;
}

template<typename Allocator>
void mixed<Allocator>::destroy() noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().~string();
      break;
    case type::ARRAY:
      as_array().~array<mixed, Allocator>();
      break;
    default: {
    }
  }
}

template<typename Allocator>
void mixed<Allocator>::clear() noexcept {
  destroy();
  type_ = type::NUL;
}

template<typename Allocator>
mixed<Allocator>::~mixed() noexcept {
  clear();
}
}

namespace impl_ {

template<class MathOperation, typename Allocator>
inline __mixed<Allocator> do_math_op_on_vars(const __mixed<Allocator> &lhs, const __mixed<Allocator> &rhs, MathOperation &&math_op) {
  if (likely(lhs.is_int() && rhs.is_int())) {
    return math_op(lhs.as_int(), rhs.as_int());
  }

  const __mixed<Allocator> arg1 = lhs.to_numeric();
  const __mixed<Allocator> arg2 = rhs.to_numeric();

  if (arg1.is_int()) {
    if (arg2.is_int()) {
      return math_op(arg1.as_int(), arg2.as_int());
    } else {
      return math_op(static_cast<double>(arg1.as_int()), arg2.as_double());
    }
  } else {
    if (arg2.is_int()) {
      return math_op(arg1.as_double(), static_cast<double>(arg2.as_int()));
    } else {
      return math_op(arg1.as_double(), arg2.as_double());
    }
  }
}

} // namespace impl_

template<typename Lhs, typename Rhs>
inline const char *conversion_php_warning_string() {
  if constexpr (std::is_same_v<Lhs, int64_t>) {
    return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %" PRIi64 " and \"%s\" (PHP7: %s, PHP8: %s)";
  } else if constexpr (std::is_same_v<Lhs, double>) {
    return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %lf and \"%s\" (PHP7: %s, PHP8: %s)";
  } else if constexpr (std::is_same_v<Rhs, int64_t>) {
    return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %" PRIi64 " (PHP7: %s, PHP8: %s)";
  } else if constexpr (std::is_same_v<Rhs, double>) {
    return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %lf (PHP7: %s, PHP8: %s)";
  } else {
    return "";
  }
}

template <typename T, typename Allocator>
inline bool less_number_string_as_php8_impl(T lhs, const __string<Allocator> &rhs) {
  auto rhs_float = 0.0;
  const auto rhs_is_string_number = rhs.try_to_float(&rhs_float);

  if (rhs_is_string_number) {
    return lhs < rhs_float;
  } else {
    return compare_strings_php_order(__string<Allocator>(lhs), rhs) < 0;
  }
}

template <typename T, typename Allocator>
inline bool less_string_number_as_php8_impl(const __string<Allocator> &lhs, T rhs) {
  auto lhs_float = 0.0;
  const auto lhs_is_string_number = lhs.try_to_float(&lhs_float);

  if (lhs_is_string_number) {
    return lhs_float < rhs;
  } else {
    return compare_strings_php_order(lhs, __string<Allocator>(rhs)) < 0;
  }
}

template <typename T, typename Allocator>
inline bool less_number_string_as_php8(bool php7_result, T lhs, const __string<Allocator> &rhs) {
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = less_number_string_as_php8_impl(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning(conversion_php_warning_string<typename std::decay<decltype(lhs)>::type, typename std::decay<decltype(rhs)>::type>(),
                lhs,
                rhs.c_str(),
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}

template <typename T, typename Allocator>
inline bool less_string_number_as_php8(bool php7_result, const __string<Allocator> &lhs, T rhs) {
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = less_string_number_as_php8_impl(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning(conversion_php_warning_string<typename std::decay<decltype(lhs)>::type, typename std::decay<decltype(rhs)>::type>(),
                lhs.c_str(),
                rhs,
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}


template <typename Allocator>
inline void swap(__mixed<Allocator> &lhs, __mixed<Allocator> &rhs) {
  lhs.swap(rhs);
}
