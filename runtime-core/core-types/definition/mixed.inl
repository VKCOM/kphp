// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/find.h"

#include "runtime-core/utils/migration-php8.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

static_assert(vk::all_of_equal(sizeof(string), sizeof(double), sizeof(array<mixed>)), "sizeof of array<mixed>, string and double must be equal");

void mixed::copy_from(const mixed &other) {
  switch (other.get_type()) {
    case type::STRING:
      new(&as_string()) string(other.as_string());
      break;
    case type::ARRAY:
      new(&as_array()) array<mixed>(other.as_array());
      break;
    default:
      storage_ = other.storage_;
  }
  type_ = other.get_type();
}

void mixed::copy_from(mixed &&other) {
  switch (other.get_type()) {
    case type::STRING:
      new(&as_string()) string(std::move(other.as_string()));
      break;
    case type::ARRAY:
      new(&as_array()) array<mixed>(std::move(other.as_array()));
      break;
    default:
      storage_ = other.storage_;
  }
  type_ = other.get_type();
}

template<typename T>
void mixed::init_from(T &&v) {
  auto type_and_value_ref = get_type_and_value_ptr(v);
  type_ = type_and_value_ref.first;
  auto *value_ptr = type_and_value_ref.second;
  using ValueType = std::decay_t<decltype(*value_ptr)>;
  new(value_ptr) ValueType(std::forward<T>(v));
}

template<typename T>
mixed &mixed::assign_from(T &&v) {
  auto type_and_value_ref = get_type_and_value_ptr(v);
  if (get_type() == type_and_value_ref.first) {
    *type_and_value_ref.second = std::forward<T>(v);
  } else {
    destroy();
    init_from(std::forward<T>(v));
  }
  return *this;
}

template<typename T, typename>
mixed::mixed(T &&v) noexcept {
  init_from(std::forward<T>(v));
}

mixed::mixed(const Unknown &u __attribute__((unused))) noexcept {
  php_assert ("Unknown used!!!" && 0);
}

mixed::mixed(const char *s, string::size_type len) noexcept :
  mixed(string{s, len}){
}

template<typename T, typename>
mixed::mixed(const Optional<T> &v) noexcept {
  auto init_from_lambda = [this](const auto &v) { this->init_from(v); };
  call_fun_on_optional_value(init_from_lambda, v);
}

template<typename T, typename>
mixed::mixed(Optional<T> &&v) noexcept {
   auto init_from_lambda = [this](auto &&v) { this->init_from(std::move(v)); };
   call_fun_on_optional_value(init_from_lambda, std::move(v));
}

mixed::mixed(const mixed &v) noexcept {
  copy_from(v);
}

mixed::mixed(mixed &&v) noexcept {
  copy_from(std::move(v));
}

mixed &mixed::operator=(const mixed &other) noexcept {
  if (this != &other) {
    destroy();
    copy_from(other);
  }
  return *this;
}

mixed &mixed::operator=(mixed &&other) noexcept {
  if (this != &other) {
    destroy();
    copy_from(std::move(other));
  }
  return *this;
}

template<typename T, typename>
mixed &mixed::operator=(T &&v) noexcept {
  return assign_from(std::forward<T>(v));
}

template<typename T, typename>
mixed &mixed::operator=(const Optional<T> &v) noexcept {
  auto assign_from_lambda = [this](const auto &v) -> mixed& { return this->assign_from(v); };
  return call_fun_on_optional_value(assign_from_lambda, v);
}

template<typename T, typename>
mixed &mixed::operator=(Optional<T> &&v) noexcept {
  auto assign_from_lambda = [this](auto &&v) -> mixed& { return this->assign_from(std::move(v)); };
  return call_fun_on_optional_value(assign_from_lambda, std::move(v));
}

mixed &mixed::assign(const char *other, string::size_type len) {
  if (get_type() == type::STRING) {
    as_string().assign(other, len);
  } else {
    destroy();
    type_ = type::STRING;
    new(&as_string()) string(other, len);
  }
  return *this;
}

const mixed mixed::operator-() const {
  mixed arg1 = to_numeric();

  if (arg1.get_type() == type::INTEGER) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

const mixed mixed::operator+() const {
  return to_numeric();
}


int64_t mixed::operator~() const {
  return ~to_int();
}


mixed &mixed::operator+=(const mixed &other) {
  if (likely (get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
    as_int() += other.as_int();
    return *this;
  }

  if (unlikely (get_type() == type::ARRAY || other.get_type() == type::ARRAY)) {
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

mixed &mixed::operator-=(const mixed &other) {
  if (likely (get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
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

mixed &mixed::operator*=(const mixed &other) {
  if (likely (get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
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

mixed &mixed::operator/=(const mixed &other) {
  if (likely (get_type() == type::INTEGER && other.get_type() == type::INTEGER)) {
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

mixed &mixed::operator%=(const mixed &other) {
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


mixed &mixed::operator&=(const mixed &other) {
  convert_to_int();
  as_int() &= other.to_int();
  return *this;
}

mixed &mixed::operator|=(const mixed &other) {
  convert_to_int();
  as_int() |= other.to_int();
  return *this;
}

mixed &mixed::operator^=(const mixed &other) {
  convert_to_int();
  as_int() ^= other.to_int();
  return *this;
}

mixed &mixed::operator<<=(const mixed &other) {
  convert_to_int();
  as_int() <<= other.to_int();
  return *this;
}

mixed &mixed::operator>>=(const mixed &other) {
  convert_to_int();
  as_int() >>= other.to_int();
  return *this;
}


mixed &mixed::operator++() {
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

const mixed mixed::operator++(int) {
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

mixed &mixed::operator--() {
  if (likely (get_type() == type::INTEGER)) {
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

const mixed mixed::operator--(int) {
  if (likely (get_type() == type::INTEGER)) {
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


bool mixed::operator!() const {
  return !to_bool();
}


mixed &mixed::append(const string &v) {
  if (unlikely (get_type() != type::STRING)) {
    convert_to_string();
  }
  as_string().append(v);
  return *this;
}

mixed &mixed::append(tmp_string v) {
  if (unlikely (get_type() != type::STRING)) {
    convert_to_string();
  }
  as_string().append(v.data, v.size);
  return *this;
}

const mixed mixed::to_numeric() const {
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


bool mixed::to_bool() const {
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

int64_t mixed::to_int() const {
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

double mixed::to_float() const {
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

const string mixed::to_string() const {
  switch (get_type()) {
    case type::NUL:
      return string();
    case type::BOOLEAN:
      return (as_bool() ? string("1", 1) : string());
    case type::INTEGER:
      return string(as_int());
    case type::FLOAT:
      return string(as_double());
    case type::STRING:
      return as_string();
    case type::ARRAY:
      php_warning("Conversion from array to string");
      return string("Array", 5);
    default:
      __builtin_unreachable();
  }
}

const array<mixed> mixed::to_array() const {
  switch (get_type()) {
    case type::NUL:
      return array<mixed>();
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
    case type::STRING: {
      array<mixed> res(array_size(1, true));
      res.push_back(*this);
      return res;
    }
    case type::ARRAY:
      return as_array();
    default:
      __builtin_unreachable();
  }
}

bool &mixed::as_bool() { return *reinterpret_cast<bool *>(&storage_); }
const bool &mixed::as_bool() const { return *reinterpret_cast<const bool *>(&storage_); }

int64_t &mixed::as_int() { return *reinterpret_cast<int64_t *>(&storage_); }
const int64_t &mixed::as_int() const { return *reinterpret_cast<const int64_t *>(&storage_); }

double &mixed::as_double() { return *reinterpret_cast<double *>(&storage_); }
const double &mixed::as_double() const { return *reinterpret_cast<const double *>(&storage_); }

string &mixed::as_string() { return *reinterpret_cast<string *>(&storage_); }
const string &mixed::as_string() const { return *reinterpret_cast<const string *>(&storage_); }

array<mixed> &mixed::as_array() { return *reinterpret_cast<array<mixed> *>(&storage_); }
const array<mixed> &mixed::as_array() const { return *reinterpret_cast<const array<mixed> *>(&storage_); }


int64_t mixed::safe_to_int() const {
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


void mixed::convert_to_numeric() {
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
      as_array().~array<mixed>();
      type_ = type::INTEGER;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void mixed::convert_to_bool() {
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
      as_array().~array<mixed>();
      type_ = type::BOOLEAN;
      as_bool() = bool_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void mixed::convert_to_int() {
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
      as_array().~array<mixed>();
      type_ = type::INTEGER;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void mixed::convert_to_float() {
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
      as_array().~array<mixed>();
      type_ = type::FLOAT;
      as_double() = float_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void mixed::convert_to_string() {
  switch (get_type()) {
    case type::NUL:
      type_ = type::STRING;
      new(&as_string()) string();
      return;
    case type::BOOLEAN:
      type_ = type::STRING;
      if (as_bool()) {
        new(&as_string()) string("1", 1);
      } else {
        new(&as_string()) string();
      }
      return;
    case type::INTEGER:
      type_ = type::STRING;
      new(&as_string()) string(as_int());
      return;
    case type::FLOAT:
      type_ = type::STRING;
      new(&as_string()) string(as_double());
      return;
    case type::STRING:
      return;
    case type::ARRAY:
      php_warning("Converting from array to string");
      as_array().~array<mixed>();
      type_ = type::STRING;
      new(&as_string()) string("Array", 5);
      return;
    default:
      __builtin_unreachable();
  }
}

const bool &mixed::as_bool(const char *function) const {
  switch (get_type()) {
    case type::BOOLEAN:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_c_str());
      return empty_value<bool>();
  }
}

const int64_t &mixed::as_int(const char *function) const {
  switch (get_type()) {
    case type::INTEGER:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_c_str());
      return empty_value<int64_t>();
  }
}

const double &mixed::as_float(const char *function) const {
  switch (get_type()) {
    case type::FLOAT:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_c_str());
      return empty_value<double>();
  }
}

const string &mixed::as_string(const char *function) const {
  switch (get_type()) {
    case type::STRING:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_c_str());
      return empty_value<string>();
  }
}

const array<mixed> &mixed::as_array(const char *function) const {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      return empty_value<array<mixed>>();
  }
}


bool &mixed::as_bool(const char *function) {
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

int64_t &mixed::as_int(const char *function) {
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

double &mixed::as_float(const char *function) {
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

string &mixed::as_string(const char *function) {
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
      return empty_value<string>();
  }
}

array<mixed> &mixed::as_array(const char *function) {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      return empty_value<array<mixed>>();
  }
}


bool mixed::is_numeric() const {
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

bool mixed::is_scalar() const {
  return get_type() != type::NUL && get_type() != type::ARRAY;
}


mixed::type mixed::get_type() const {
  return type_;
}

bool mixed::is_null() const {
  return get_type() == type::NUL;
}

bool mixed::is_bool() const {
  return get_type() == type::BOOLEAN;
}

bool mixed::is_int() const {
  return get_type() == type::INTEGER;
}

bool mixed::is_float() const {
  return get_type() == type::FLOAT;
}

bool mixed::is_string() const {
  return get_type() == type::STRING;
}

bool mixed::is_array() const {
  return get_type() == type::ARRAY;
}


inline const char *mixed::get_type_c_str() const {
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

inline const string mixed::get_type_str() const {
  return string(get_type_c_str());
}


bool mixed::empty() const {
  return !to_bool();
}

int64_t mixed::count() const {
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

template<class T1, class T2>
inline int64_t spaceship(const T1 &lhs, const T2 &rhs);

int64_t mixed::compare(const mixed &rhs) const {
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

void mixed::swap(mixed &other) {
  ::swap(type_, other.type_);
  ::swap(as_double(), other.as_double());
}


mixed &mixed::operator[](int64_t int_key) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      php_warning("Writing to string by offset is't supported");
      return empty_value<mixed>();
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
      return empty_value<mixed>();
    }
  }
  return as_array()[int_key];
}

mixed &mixed::operator[](const string &string_key) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      php_warning("Writing to string by offset is't supported");
      return empty_value<mixed>();
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
      return empty_value<mixed>();
    }
  }

  return as_array()[string_key];
}

mixed &mixed::operator[](tmp_string string_key) {
  if (get_type() == type::ARRAY) {
    return as_array()[string_key];
  }
  return (*this)[materialize_tmp_string(string_key)];
}

mixed &mixed::operator[](const mixed &v) {
  switch (v.get_type()) {
    case type::NUL:
      return (*this)[string()];
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

mixed &mixed::operator[](double double_key) {
  return (*this)[static_cast<int64_t>(double_key)];
}

mixed &mixed::operator[](const array<mixed>::const_iterator &it) {
  return as_array()[it];
}

mixed &mixed::operator[](const array<mixed>::iterator &it) {
  return as_array()[it];
}


void mixed::set_value(int64_t int_key, const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      auto rhs_string = v.to_string();
      if (rhs_string.empty()) {
        php_warning("Cannot assign an empty string to a string offset, index = %" PRIi64, int_key);
        return;
      }

      const char c = rhs_string[0];

      if (int_key >= 0) {
        const string::size_type l = as_string().size();
        if (int_key >= l) {
          as_string().append(string::unsafe_cast_to_size_type(int_key + 1 - l), ' ');
        } else {
          as_string().make_not_shared();
        }

        as_string()[static_cast<string::size_type>(int_key)] = c;
      } else {
        php_warning("%" PRIi64 " is illegal offset for string", int_key);
      }
      return;
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
      return;
    }
  }
  return as_array().set_value(int_key, v);
}

void mixed::set_value(const string &string_key, const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
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

      const string::size_type l = as_string().size();
      if (int_val >= l) {
        as_string().append(string::unsafe_cast_to_size_type(int_val + 1 - l), ' ');
      } else {
        as_string().make_not_shared();
      }

      as_string()[static_cast<string::size_type>(int_val)] = c;
      return;
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
      return;
    }
  }

  return as_array().set_value(string_key, v);
}

void mixed::set_value(const string &string_key, const mixed &v, int64_t precomuted_hash) {
  return get_type() == type::ARRAY ? as_array().set_value(string_key, v, precomuted_hash) : set_value(string_key, v);
}

void mixed::set_value(tmp_string string_key, const mixed &v) {
  // TODO: as with arrays, avoid eager tmp_string->string conversion
  set_value(materialize_tmp_string(string_key), v);
}

void mixed::set_value(const mixed &v, const mixed &value) {
  switch (v.get_type()) {
    case type::NUL:
      return set_value(string(), value);
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

void mixed::set_value(double double_key, const mixed &value) {
  set_value(static_cast<int64_t>(double_key), value);
}

void mixed::set_value(const array<mixed>::const_iterator &it) {
  return as_array().set_value(it);
}

void mixed::set_value(const array<mixed>::iterator &it) {
  return as_array().set_value(it);
}


const mixed mixed::get_value(int64_t int_key) const {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      if (int_key < 0 || int_key >= as_string().size()) {
        return string();
      }
      return string(1, as_string()[static_cast<string::size_type>(int_key)]);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string().c_str(), get_type_c_str(), int_key);
    }
    return mixed();
  }

  return as_array().get_value(int_key);
}

const mixed mixed::get_value(const string &string_key) const {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      int64_t int_val = 0;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        int_val = string_key.to_int();
      }
      if (int_val < 0 || int_val >= as_string().size()) {
        return string();
      }
      return string(1, as_string()[static_cast<string::size_type>(int_val)]);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
    }
    return mixed();
  }

  return as_array().get_value(string_key);
}

const mixed mixed::get_value(const string &string_key, int64_t precomuted_hash) const {
  return get_type() == type::ARRAY ? as_array().get_value(string_key, precomuted_hash) : get_value(string_key);
}

const mixed mixed::get_value(tmp_string string_key) const {
  if (get_type() == type::ARRAY) {
    // fast path: arrays can handle a tmp_string lookup efficiently
    return as_array().get_value(string_key);
  }
  // TODO: make other lookups efficient too (like string from numeric tmp_string)?
  return get_value(materialize_tmp_string(string_key));
}

const mixed mixed::get_value(const mixed &v) const {
  switch (v.get_type()) {
    case type::NUL:
      return get_value(string());
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

const mixed mixed::get_value(double double_key) const {
  return get_value(static_cast<int64_t>(double_key));
}

const mixed mixed::get_value(const array<mixed>::const_iterator &it) const {
  return as_array().get_value(it);
}

const mixed mixed::get_value(const array<mixed>::iterator &it) const {
  return as_array().get_value(it);
}


void mixed::push_back(const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      return;
    }
  }

  return as_array().push_back(v);
}

const mixed mixed::push_back_return(const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      return empty_value<mixed>();
    }
  }

  return as_array().push_back_return(v);
}


bool mixed::isset(int64_t int_key) const {
  if (unlikely (get_type() != type::ARRAY)) {
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

template <class ...MaybeHash>
bool mixed::isset(const string &string_key, MaybeHash ...maybe_hash) const {
  if (unlikely (get_type() != type::ARRAY)) {
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

bool mixed::isset(const mixed &v) const {
  switch (v.get_type()) {
    case type::NUL:
      return isset(string());
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

bool mixed::isset(double double_key) const {
  return isset(static_cast<int64_t>(double_key));
}

void mixed::unset(int64_t int_key) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  as_array().unset(int_key);
}

template <class ...MaybeHash>
void mixed::unset(const string &string_key, MaybeHash ...maybe_hash) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  as_array().unset(string_key, maybe_hash...);
}

void mixed::unset(const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  switch (v.get_type()) {
    case type::NUL:
      as_array().unset(string());
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

void mixed::unset(double double_key) {
  unset(static_cast<int64_t>(double_key));
}

array<mixed>::const_iterator mixed::begin() const {
  if (likely (get_type() == type::ARRAY)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_c_str(), to_string().c_str());
  return array<mixed>::const_iterator();
}

array<mixed>::const_iterator mixed::end() const {
  if (likely (get_type() == type::ARRAY)) {
    return as_array().end();
  }
  return array<mixed>::const_iterator();
}


array<mixed>::iterator mixed::begin() {
  if (likely (get_type() == type::ARRAY)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_c_str(), to_string().c_str());
  return array<mixed>::iterator();
}

array<mixed>::iterator mixed::end() {
  if (likely (get_type() == type::ARRAY)) {
    return as_array().end();
  }
  return array<mixed>::iterator();
}


int64_t mixed::get_reference_counter() const {
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

void mixed::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
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

inline bool mixed::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
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

inline void mixed::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
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

size_t mixed::estimate_memory_usage() const {
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

void mixed::reset_empty_values() noexcept {
  empty_value<bool>();
  empty_value<int64_t>();
  empty_value<double>();
  empty_value<string>();
  empty_value<mixed>();
  empty_value<array<mixed>>();
}

template<typename T>
T &mixed::empty_value() noexcept {
  static_assert(vk::is_type_in_list<T, bool, int64_t, double, string, mixed, array<mixed>>{}, "unsupported type");
  static T value;
  value = T{};
  return value;
}

namespace impl_ {

template<class MathOperation>
inline mixed do_math_op_on_vars(const mixed &lhs, const mixed &rhs, MathOperation &&math_op) {
  if (likely(lhs.is_int() && rhs.is_int())) {
    return math_op(lhs.as_int(), rhs.as_int());
  }

  const mixed arg1 = lhs.to_numeric();
  const mixed arg2 = rhs.to_numeric();

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

inline mixed operator+(const mixed &lhs, const mixed &rhs) {
  if (lhs.is_array() && rhs.is_array()) {
    return lhs.as_array() + rhs.as_array();
  }

  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 + arg2; });
}

inline mixed operator-(const mixed &lhs, const mixed &rhs) {
  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 - arg2; });
}

inline mixed operator*(const mixed &lhs, const mixed &rhs) {
  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 * arg2; });
}

inline mixed operator-(const string &lhs) {
  mixed arg1 = lhs.to_numeric();

  if (arg1.is_int()) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

inline mixed operator+(const string &lhs) {
  return lhs.to_numeric();
}

inline int64_t operator&(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() & rhs.to_int();
}

inline int64_t operator|(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() | rhs.to_int();
}

inline int64_t operator^(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() ^ rhs.to_int();
}

inline int64_t operator<<(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() << rhs.to_int();
}

inline int64_t operator>>(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() >> rhs.to_int();
}

template <typename Arg1, typename Arg2>
inline const char *conversion_php_warning_string() {
  return "";
}

template<>
inline const char *conversion_php_warning_string<int64_t, string>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %" PRIi64 " and \"%s\" (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<double, string>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %lf and \"%s\" (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<string, int64_t>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %" PRIi64 " (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<string, double>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %lf (PHP7: %s, PHP8: %s)";
}

template <typename T>
inline bool less_number_string_as_php8_impl(T lhs, const string &rhs) {
  auto rhs_float = 0.0;
  const auto rhs_is_string_number = rhs.try_to_float(&rhs_float);

  if (rhs_is_string_number) {
    return lhs < rhs_float;
  } else {
    return compare_strings_php_order(string(lhs), rhs) < 0;
  }
}

template <typename T>
inline bool less_string_number_as_php8_impl(const string &lhs, T rhs) {
  auto lhs_float = 0.0;
  const auto lhs_is_string_number = lhs.try_to_float(&lhs_float);

  if (lhs_is_string_number) {
    return lhs_float < rhs;
  } else {
    return compare_strings_php_order(lhs, string(rhs)) < 0;
  }
}

template <typename T>
inline bool less_number_string_as_php8(bool php7_result, T lhs, const string &rhs) {
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

template <typename T>
inline bool less_string_number_as_php8(bool php7_result, const string &lhs, T rhs) {
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

inline bool operator<(const mixed &lhs, const mixed &rhs) {
  const auto res = lhs.compare(rhs) < 0;

  if (rhs.is_string()) {
    if (lhs.is_int()) {
      return less_number_string_as_php8(res, lhs.to_int(), rhs.to_string());
    } else if (lhs.is_float()) {
      return less_number_string_as_php8(res, lhs.to_float(), rhs.to_string());
    }
  } else if (lhs.is_string()) {
    if (rhs.is_int()) {
      return less_string_number_as_php8(res, lhs.to_string(), rhs.to_int());
    } else if (rhs.is_float()) {
      return less_string_number_as_php8(res, lhs.to_string(), rhs.to_float());
    }
  }

  return res;
}

inline bool operator<=(const mixed &lhs, const mixed &rhs) {
  return !(rhs < lhs);
}

inline void swap(mixed &lhs, mixed &rhs) {
  lhs.swap(rhs);
}

template<class T>
inline string_buffer &operator<<(string_buffer &sb, const Optional<T> &v) {
  auto write_lambda = [&sb](const auto &v) -> string_buffer& { return sb << v; };
  return call_fun_on_optional_value(write_lambda, v);
}

inline string_buffer &operator<<(string_buffer &sb, const mixed &v) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return sb;
    case mixed::type::BOOLEAN:
      return sb << v.as_bool();
    case mixed::type::INTEGER:
      return sb << v.as_int();
    case mixed::type::FLOAT:
      return sb << string(v.as_double());
    case mixed::type::STRING:
      return sb << v.as_string();
    case mixed::type::ARRAY:
      php_warning("Conversion from array to string");
      return sb.append("Array", 5);
    default:
      __builtin_unreachable();
  }
}
