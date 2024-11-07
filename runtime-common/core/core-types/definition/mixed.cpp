// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/wrappers/likely.h"
#include "runtime-common/core/runtime-core.h"

namespace {

string to_string_without_warning(const mixed &m) {
  switch (m.get_type()) {
    case mixed::type::NUL:
      return string();
    case mixed::type::BOOLEAN:
      return (m.as_bool() ? string("1", 1) : string());
    case mixed::type::INTEGER:
      return string(m.as_int());
    case mixed::type::FLOAT:
      return string(m.as_double());
    case mixed::type::STRING:
      return m.as_string();
    case mixed::type::ARRAY:
      return string("Array", 5);
    case mixed::type::OBJECT: {
      const char *s = m.get_type_or_class_name();
      return string(s, strlen(s));
    }
    default:
      __builtin_unreachable();
  }
}

} // namespace

void mixed::copy_from(const mixed &other) {
  switch (other.get_type()) {
    case type::STRING:
      new(&as_string()) string(other.as_string());
      break;
    case type::ARRAY:
      new(&as_array()) array<mixed>(other.as_array());
      break;
    case type::OBJECT: {
      new (&storage_) vk::intrusive_ptr<may_be_mixed_base>(*reinterpret_cast<const vk::intrusive_ptr<may_be_mixed_base> *>(&other.storage_));
      break;
    }
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
    case type::OBJECT: {
      storage_ = other.storage_;
      other.storage_ = 0;
      break;
    }
    default:
      storage_ = other.storage_;
  }
  type_ = other.get_type();
}

mixed::mixed(const mixed &v) noexcept {
  copy_from(v);
}

mixed::mixed(mixed &&v) noexcept {
  copy_from(std::move(v));
}

mixed::mixed(const Unknown &u __attribute__((unused))) noexcept {
  php_assert ("Unknown used!!!" && 0);
}

mixed::mixed(const char *s, string::size_type len) noexcept :
  mixed(string{s, len}){
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
    case type::OBJECT:
      php_warning("Can't apply operator ++ to %s", get_type_or_class_name());
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
    case type::OBJECT:
      php_warning("Can't apply operator ++ to %s", get_type_or_class_name());
      return *this;
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
    case type::OBJECT:
      php_warning("Can't apply operator -- to %s", get_type_or_class_name());
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
    case type::OBJECT:
      php_warning("Can't apply operator -- to %s", get_type_or_class_name());
      return *this;
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
    case type::OBJECT:
      php_warning("Wrong conversion from %s to number", get_type_or_class_name());
      return (as_object() ? 1 : 0);
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
    case type::OBJECT:
      return (bool)as_object();
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
    case type::OBJECT:
      php_warning("Wrong conversion from %s to int", get_type_or_class_name());
      return (as_object() ? 1 : 0);
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to float", get_type_or_class_name());
      return (as_object() ? 1.0 : 0.0);
    }
    default:
      __builtin_unreachable();
  }
}


const string mixed::to_string() const {
  switch (get_type()) {
    case mixed::type::NUL:
    case mixed::type::BOOLEAN:
    case mixed::type::INTEGER:
    case mixed::type::FLOAT:
    case mixed::type::STRING:
      break;
    case type::ARRAY:
      php_warning("Conversion from array to string");
      break;
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to string", get_type_or_class_name());
      break;
    }
    default:
      __builtin_unreachable();
  }
  return to_string_without_warning(*this);
}

const array<mixed> mixed::to_array() const {
  switch (get_type()) {
    case type::NUL:
      return array<mixed>();
    case type::BOOLEAN:
    case type::INTEGER:
    case type::FLOAT:
    case type::STRING:
    case type::OBJECT: {
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to int", get_type_or_class_name());
      return (as_object() ? 1 : 0);
    }
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to number", get_type_or_class_name());
      const int64_t int_val = (as_object() ? 1 : 0);
      destroy();
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
    case type::OBJECT: {
      const bool bool_val = static_cast<bool>(as_object());
      destroy();
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to int", get_type_or_class_name());
      const int64_t int_val = (as_object() ? 1 : 0);
      destroy();
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to float", get_type_or_class_name());
      const double float_val = (as_object() ? 1.0 : 0.0);
      destroy();
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
    case type::OBJECT: {
      php_warning("Wrong conversion from %s to string", get_type_or_class_name());
      string s = string(get_type_or_class_name(), strlen(get_type_or_class_name()));
      destroy();
      type_ = type::STRING;
      new (&as_string()) string(std::move(s));
      return;
    }
    default:
      __builtin_unreachable();
  }
}

const bool &mixed::as_bool(const char *function) const {
  switch (get_type()) {
    case type::BOOLEAN:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_or_class_name());
      return empty_value<bool>();
  }
}

const int64_t &mixed::as_int(const char *function) const {
  switch (get_type()) {
    case type::INTEGER:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_or_class_name());
      return empty_value<int64_t>();
  }
}

const double &mixed::as_float(const char *function) const {
  switch (get_type()) {
    case type::FLOAT:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_or_class_name());
      return empty_value<double>();
  }
}

const string &mixed::as_string(const char *function) const {
  switch (get_type()) {
    case type::STRING:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_or_class_name());
      return empty_value<string>();
  }
}

const array<mixed> &mixed::as_array(const char *function) const {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_or_class_name());
      return empty_value<array<mixed>>();
  }
}


bool &mixed::as_bool(const char *function) {
  switch (get_type()) {
    case type::NUL:
      convert_to_bool();
      [[fallthrough]];
    case type::BOOLEAN:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_or_class_name());
      return empty_value<bool>();
  }
}

int64_t &mixed::as_int(const char *function) {
  switch (get_type()) {
    case type::NUL:
      [[fallthrough]];
    case type::BOOLEAN:
      [[fallthrough]];
    case type::FLOAT:
      [[fallthrough]];
    case type::STRING:
      convert_to_int();
      [[fallthrough]];
    case type::INTEGER:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_or_class_name());
      return empty_value<int64_t>();
  }
}

double &mixed::as_float(const char *function) {
  switch (get_type()) {
    case type::NUL:
      [[fallthrough]];
    case type::BOOLEAN:
      [[fallthrough]];
    case type::INTEGER:
      [[fallthrough]];
    case type::STRING:
      convert_to_float();
      [[fallthrough]];
    case type::FLOAT:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_or_class_name());
      return empty_value<double>();
  }
}

string &mixed::as_string(const char *function) {
  switch (get_type()) {
    case type::NUL:
      [[fallthrough]];
    case type::BOOLEAN:
      [[fallthrough]];
    case type::INTEGER:
      [[fallthrough]];
    case type::FLOAT:
      convert_to_string();
      [[fallthrough]];
    case type::STRING:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_or_class_name());
      return empty_value<string>();
  }
}

array<mixed> &mixed::as_array(const char *function) {
  switch (get_type()) {
    case type::ARRAY:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_or_class_name());
      return empty_value<array<mixed>>();
  }
}


bool mixed::is_numeric() const {
  switch (get_type()) {
    case type::INTEGER:
      [[fallthrough]];
    case type::FLOAT:
      return true;
    case type::STRING:
      return as_string().is_numeric();
    default:
      return false;
  }
}

bool mixed::is_scalar() const {
  return get_type() != type::NUL && get_type() != type::ARRAY && get_type() != type::OBJECT;
}



const char *mixed::get_type_c_str() const {
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
    case type::OBJECT:
      return "object";
    default:
      __builtin_unreachable();
  }
}

const char *mixed::get_type_or_class_name() const {
  switch (get_type()) {
    case type::OBJECT:
      return as_object()->get_class();
    default:
      return get_type_c_str();
  }
}

const string mixed::get_type_str() const {
  return string(get_type_c_str());
}

// TODO
// Should we warn more precisely: "Class XXX does not implement \\ArrayAccess" or just
// "Cannot use XXX as array, index = YYY" will be OK?
// Note on usage: should check `if (type_ == type::OBJECT)` or not?
// On the one hand, it's redundant. On the other hand, it's fast check
std::pair<class_instance<C$ArrayAccess>, bool> try_as_array_access(const mixed &m) noexcept {
  using T = class_instance<C$ArrayAccess>;
  
  // For now, it does dynamic cast twice
  // We can get rid of one of them
  if (likely(m.is_a<C$ArrayAccess>())) {
    return {from_mixed<T>(m, string()), true};
  }

  return {T{}, false};
}

bool mixed::empty_on(const mixed &key) const {
  if (type_ == type::OBJECT) {
    if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
      return !f$ArrayAccess$$offsetExists(as_aa, key) || f$ArrayAccess$$offsetGet(as_aa, key).empty();
    }
  }

  return get_value(key).empty();
}

bool mixed::empty_on(const string &key) const {
  if (type_ == type::OBJECT) {
    if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
      return !f$ArrayAccess$$offsetExists(as_aa, key) || f$ArrayAccess$$offsetGet(as_aa, key).empty();
    }
  }

  return get_value(key).empty();
}

bool mixed::empty_on(const string &key, int64_t precomputed_hash) const {
  if (type_ == type::OBJECT) {
    if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
      return !f$ArrayAccess$$offsetExists(as_aa, key) || f$ArrayAccess$$offsetGet(as_aa, key).empty();
    }
  }

  return get_value(key, precomputed_hash).empty();
}

bool mixed::empty_on(const array<mixed>::iterator &key) const {
  return get_value(key).empty();
}

bool mixed::empty_on(const array<mixed>::const_iterator &key) const {
  return get_value(key).empty();
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
    case type::OBJECT:
      php_warning("count(): Parameter is %s, but an array expected", get_type_or_class_name());
      return (as_object() ? 1 : 0);
    default:
      __builtin_unreachable();
  }
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
  if (unlikely(is_object() || rhs.is_object())) {
    php_warning("Unsupported operand types for operator < or <= (%s and %s)", get_type_or_class_name(), get_type_or_class_name());
    return is_object() ? 1 : -1;
  }

  return three_way_comparison(to_float(), rhs.to_float());
}

mixed &mixed::operator[](int64_t int_key) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::STRING) {
      php_warning("Writing to string by offset isn't supported");
      return empty_value<mixed>();
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string_without_warning(*this).c_str(), get_type_or_class_name(), int_key);
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
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string_without_warning(*this).c_str(), get_type_or_class_name(), string_key.c_str());
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
    case type::OBJECT:
      php_warning("Illegal offset type %s", v.get_type_or_class_name());
      return (*this)[(as_object() ? 1 : 0)];
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

mixed mixed::set_value_return(const mixed &key, const mixed &val) {
  if (get_type() == type::OBJECT) {
    set_value(key, val);
    return val;
  }
  return (*this)[key] = val;
}

mixed mixed::set_value_return(const string &key, const mixed &val) {
  if (get_type() == type::OBJECT) {
    set_value(key, val);
    return val;
  }
  return (*this)[key] = val;
}

mixed mixed::set_value_return(const array<mixed>::iterator &key, const mixed &val) {
  return (*this)[key] = val;
}

mixed mixed::set_value_return(const array<mixed>::const_iterator &key, const mixed &val) {
  return (*this)[key] = val;
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

    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetSet(as_aa, int_key, v);
        return;
      }
    }

    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string_without_warning(*this).c_str(), get_type_or_class_name(), int_key);
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

    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetSet(as_aa, string_key, v);
        return;
      }
    }
    if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string_without_warning(*this).c_str(), get_type_or_class_name(), string_key.c_str());
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
    case type::OBJECT:
      php_warning("Illegal offset type %s", v.get_type_or_class_name());
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

    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        return f$ArrayAccess$$offsetGet(as_aa, int_key);
      }
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %" PRIi64, to_string_without_warning(*this).c_str(), get_type_or_class_name(), int_key);
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

    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        return f$ArrayAccess$$offsetGet(as_aa, string_key);
      }
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string_without_warning(*this).c_str(), get_type_or_class_name(), string_key.c_str());
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
    case type::OBJECT:
      php_warning("Illegal offset type %s", v.get_type_or_class_name());
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
    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetSet(as_aa, Optional<bool>{}, v);
        return;
      }
    } else if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_or_class_name());
      return;
    }
  }

  return as_array().push_back(v);
}

const mixed mixed::push_back_return(const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetSet(as_aa, Optional<bool>{}, v);
        return v;
      }
    } else if (get_type() == type::NUL || (get_type() == type::BOOLEAN && !as_bool())) {
      type_ = type::ARRAY;
      new(&as_array()) array<mixed>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_or_class_name());
      return empty_value<mixed>();
    }
  }

  return as_array().push_back_return(v);
}

bool mixed::isset(int64_t int_key) const {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        return f$ArrayAccess$$offsetExists(as_aa, int_key);
      }
    }
    if (get_type() == type::STRING) {
      int_key = as_string().get_correct_index(int_key);
      return as_string().isset(int_key);
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in isset", get_type_or_class_name());
    }
    return false;
  }

  return as_array().isset(int_key);
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
    case type::OBJECT:
      php_warning("Illegal offset type %s", get_type_or_class_name());
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
    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetUnset(as_aa, int_key);
        return;
      }
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_or_class_name());
    }
    return;
  }

  as_array().unset(int_key);
}


void mixed::unset(const mixed &v) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() == type::OBJECT) {
      if (auto [as_aa, succ] = try_as_array_access(*this); succ) {
        f$ArrayAccess$$offsetUnset(as_aa, v);
      }
      return;
    }

    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_or_class_name());
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
    case type::OBJECT:
      php_warning("Illegal offset type %s", v.get_type_or_class_name());
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
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_or_class_name(), to_string_without_warning(*this).c_str());
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
  php_warning("Invalid argument supplied for foreach(), %s (string representation - \"%s\") is given", get_type_or_class_name(), to_string_without_warning(*this).c_str());
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
    case type::OBJECT:
      return as_object()->get_refcnt();
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
    case type::OBJECT:
      return as_object()->set_refcnt(ref_cnt_value);
    default:
      __builtin_unreachable();
  }
}

bool mixed::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
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
    case type::OBJECT:
      return static_cast<int>(as_object()->get_refcnt()) == ref_cnt_value;
    default:
      __builtin_unreachable();
  }
}

void mixed::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().force_destroy(expected_ref_cnt);
      break;
    case type::ARRAY:
      as_array().force_destroy(expected_ref_cnt);
      break;
    case type::OBJECT:
      php_warning("Objects (%s) are not supported in confdata", get_type_or_class_name());
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
    case type::OBJECT:
      php_warning("Objects (%s) are not supported in confdata", get_type_or_class_name());
      return 0;
    default:
      __builtin_unreachable();
  }
}

namespace impl_ {

template<class MathOperation>
mixed do_math_op_on_vars(const mixed &lhs, const mixed &rhs, MathOperation &&math_op) {
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

mixed operator+(const mixed &lhs, const mixed &rhs) {
  if (lhs.is_array() && rhs.is_array()) {
    return lhs.as_array() + rhs.as_array();
  }

  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 + arg2; });
}

mixed operator-(const mixed &lhs, const mixed &rhs) {
  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 - arg2; });
}

mixed operator*(const mixed &lhs, const mixed &rhs) {
  return impl_::do_math_op_on_vars(lhs, rhs, [](const auto &arg1, const auto &arg2) { return arg1 * arg2; });
}

mixed operator-(const string &lhs) {
  mixed arg1 = lhs.to_numeric();

  if (arg1.is_int()) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

mixed operator+(const string &lhs) {
  return lhs.to_numeric();
}

int64_t operator&(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() & rhs.to_int();
}

int64_t operator|(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() | rhs.to_int();
}

int64_t operator^(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() ^ rhs.to_int();
}

int64_t operator<<(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() << rhs.to_int();
}

int64_t operator>>(const mixed &lhs, const mixed &rhs) {
  return lhs.to_int() >> rhs.to_int();
}


bool operator<(const mixed &lhs, const mixed &rhs) {
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

bool operator<=(const mixed &lhs, const mixed &rhs) {
  return !(rhs < lhs);
}

string_buffer &operator<<(string_buffer &sb, const mixed &v) {
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
    case mixed::type::OBJECT: {
      const char *s = v.get_type_or_class_name();
      php_warning("Conversion from %s to string", s);
      return sb.append(s, strlen(s));
    }
    default:
      __builtin_unreachable();
  }
}

void mixed::destroy() noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().~string();
      break;
    case type::ARRAY:
      as_array().~array<mixed>();
      break;
    case type::OBJECT: {
      reinterpret_cast<vk::intrusive_ptr<may_be_mixed_base>*>(&storage_)->~intrusive_ptr<may_be_mixed_base>();
      storage_ = 0;
      break;
    }
    default: {
    }
  }
}

void mixed::clear() noexcept {
  destroy();
  type_ = type::NUL;
}

// Don't move this destructor to the headers, it spoils addr2line traces
mixed::~mixed() noexcept {
  clear();
}

static_assert(sizeof(mixed) == SIZEOF_MIXED, "sizeof(mixed) at runtime doesn't match compile-time");
static_assert(sizeof(Unknown) == SIZEOF_UNKNOWN, "sizeof(Unknown) at runtime doesn't match compile-time");
