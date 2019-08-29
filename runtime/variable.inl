#pragma once

#include "common/algorithms/find.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

static_assert(vk::all_of_equal(sizeof(string), sizeof(double), sizeof(array<var>)), "sizeof of array<var>, string and double must be equal");

bool empty_bool __attribute__ ((weak));
int empty_int __attribute__ ((weak));
double empty_float __attribute__ ((weak));
string empty_string __attribute__ ((weak));

array<var> empty_array_var __attribute__ ((weak));
var empty_var __attribute__ ((weak));

void var::copy_from(const var &other) {
  switch (other.type) {
    case STRING_TYPE:
      new(&as_string()) string(other.as_string());
      break;
    case ARRAY_TYPE:
      new(&as_array()) array<var>(other.as_array());
      break;
    default:
      storage = other.storage;
  }
  type = other.type;
}

void var::copy_from(var &&other) {
  switch (other.type) {
    case STRING_TYPE:
      new(&as_string()) string(std::move(other.as_string()));
      break;
    case ARRAY_TYPE:
      new(&as_array()) array<var>(std::move(other.as_array()));
      break;
    default:
      storage = other.storage;
  }

  type = other.type;
  other.type = NULL_TYPE;
}

var::var(const Unknown &u __attribute__((unused))) :
  type(NULL_TYPE) {
  php_assert ("Unknown used!!!" && 0);
}

var::var(bool b) :
  type(BOOLEAN_TYPE) {
  as_bool() = b;
}

var::var(int i) :
  type(INTEGER_TYPE) {
  as_int() = i;
}

var::var(double f) :
  type(FLOAT_TYPE) {
  as_double() = f;
}

var::var(const string &s_) :
  type(STRING_TYPE) {
  new(&as_string()) string(s_);
}

var::var(string &&s_) :
  type(STRING_TYPE) {
  new(&as_string()) string(std::move(s_));
}

var::var(const char *s_, int len) :
  type(STRING_TYPE) {
  new(&as_string()) string(s_, len);
}

template<class T, class>
var::var(const array<T> &a_):
  type(ARRAY_TYPE) {
  new(&as_array()) array<var>(a_);
}

template<class T, class>
var::var(array<T> &&a_):
  type(ARRAY_TYPE) {
  new(&as_array()) array<var>(std::move(a_));
}

var::var(const OrFalse<int> &v) {
  if (v.bool_value) {
    type = INTEGER_TYPE;
    as_int() = v.value;
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

var::var(const OrFalse<double> &v) {
  if (v.bool_value) {
    type = FLOAT_TYPE;
    as_double() = v.value;
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

var::var(const OrFalse<string> &v) {
  if (v.bool_value) {
    type = STRING_TYPE;
    new(&as_string()) string(v.value);
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

var::var(OrFalse<string> &&v) {
  if (v.bool_value) {
    v.bool_value = false;
    type = STRING_TYPE;
    new(&as_string()) string(std::move(v.value));
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

template<class T, class>
var::var(const OrFalse<array<T>> &v) {
  if (v.bool_value) {
    type = ARRAY_TYPE;
    new(&as_array()) array<var>(v.value);
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

template<class T, class>
var::var(OrFalse<array<T>> &&v) {
  if (v.bool_value) {
    v.bool_value = false;
    type = ARRAY_TYPE;
    new(&as_array()) array<var>(std::move(v.value));
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
}

var::var(const var &v) {
  copy_from(v);
}

var::var(var &&v) {
  copy_from(std::move(v));
}

var &var::operator=(bool other) {
  if (type != BOOLEAN_TYPE) {
    destroy();
    type = BOOLEAN_TYPE;
  }
  as_bool() = other;
  return *this;
}

var &var::operator=(int other) {
  if (type != INTEGER_TYPE) {
    destroy();
    type = INTEGER_TYPE;
  }
  as_int() = other;
  return *this;
}

var &var::operator=(double other) {
  if (type != FLOAT_TYPE) {
    destroy();
    type = FLOAT_TYPE;
  }
  as_double() = other;
  return *this;
}

var &var::operator=(const string &other) {
  if (type == STRING_TYPE) {
    as_string() = other;
  } else {
    destroy();
    type = STRING_TYPE;
    new(&as_string()) string(other);
  }
  return *this;
}

var &var::operator=(string &&other) {
  if (type == STRING_TYPE) {
    as_string() = std::move(other);
  } else {
    destroy();
    type = STRING_TYPE;
    new(&as_string()) string(std::move(other));
  }
  return *this;
}

var &var::assign(const char *other, int len) {
  if (type == STRING_TYPE) {
    as_string().assign(other, len);
  } else {
    destroy();
    type = STRING_TYPE;
    new(&as_string()) string(other, len);
  }
  return *this;
}

template<class T, class>
var &var::operator=(const array<T> &other) {
  if (type == ARRAY_TYPE) {
    as_array() = other;
  } else {
    destroy();
    type = ARRAY_TYPE;
    new(&as_array()) array<var>(other);
  }
  return *this;
}

template<class T, class>
var &var::operator=(array<T> &&other) {
  if (type == ARRAY_TYPE) {
    as_array() = std::move(other);
  } else {
    destroy();
    type = ARRAY_TYPE;
    new(&as_array()) array<var>(std::move(other));
  }
  return *this;
}

var &var::operator=(const var &other) {
  if (this != &other) {
    destroy();
    copy_from(other);
  }
  return *this;
}

var &var::operator=(var &&other) {
  if (this != &other) {
    destroy();
    copy_from(std::move(other));
  }
  return *this;
}


var &var::operator=(const OrFalse<int> &other) {
  destroy();

  if (other.bool_value) {
    type = INTEGER_TYPE;
    as_int() = other.value;
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }

  return *this;
}

var &var::operator=(const OrFalse<double> &other) {
  destroy();

  if (other.bool_value) {
    type = FLOAT_TYPE;
    as_double() = other.value;
  } else {
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }

  return *this;
}

var &var::operator=(const OrFalse<string> &other) {
  if (other.bool_value) {
    *this = other.value;
  } else {
    destroy();
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
  return *this;
}

var &var::operator=(OrFalse<string> &&other) {
  if (other.bool_value) {
    other.bool_value = false;
    *this = std::move(other.value);
  } else {
    destroy();
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
  return *this;
}

template<class T, class>
var &var::operator=(const OrFalse<array<T>> &other) {
  if (other.bool_value) {
    *this = other.value;
  } else {
    destroy();
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
  return *this;
}

template<class T, class>
var &var::operator=(OrFalse<array<T>> &&other) {
  if (other.bool_value) {
    other.bool_value = false;
    *this = std::move(other.value);
  } else {
    destroy();
    type = BOOLEAN_TYPE;
    as_bool() = false;
  }
  return *this;
}

const var var::operator-() const {
  var arg1 = to_numeric();

  if (arg1.type == INTEGER_TYPE) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

const var var::operator+() const {
  return to_numeric();
}


int var::operator~() const {
  return ~to_int();
}


var &var::operator+=(const var &other) {
  if (likely (type == INTEGER_TYPE && other.type == INTEGER_TYPE)) {
    as_int() += other.as_int();
    return *this;
  }

  if (unlikely (type == ARRAY_TYPE || other.type == ARRAY_TYPE)) {
    if (type == ARRAY_TYPE && other.type == ARRAY_TYPE) {
      as_array() += other.as_array();
    } else {
      php_warning("Unsupported operand types for operator += (%s and %s)", get_type_c_str(), other.get_type_c_str());
    }
    return *this;
  }

  convert_to_numeric();
  const var arg2 = other.to_numeric();

  if (type == INTEGER_TYPE) {
    if (arg2.type == INTEGER_TYPE) {
      as_int() += arg2.as_int();
    } else {
      type = FLOAT_TYPE;
      as_double() = as_int() + arg2.as_double();
    }
  } else {
    if (arg2.type == INTEGER_TYPE) {
      as_double() += arg2.as_int();
    } else {
      as_double() += arg2.as_double();
    }
  }

  return *this;
}

var &var::operator-=(const var &other) {
  if (likely (type == INTEGER_TYPE && other.type == INTEGER_TYPE)) {
    as_int() -= other.as_int();
    return *this;
  }

  convert_to_numeric();
  const var arg2 = other.to_numeric();

  if (type == INTEGER_TYPE) {
    if (arg2.type == INTEGER_TYPE) {
      as_int() -= arg2.as_int();
    } else {
      type = FLOAT_TYPE;
      as_double() = as_int() - arg2.as_double();
    }
  } else {
    if (arg2.type == INTEGER_TYPE) {
      as_double() -= arg2.as_int();
    } else {
      as_double() -= arg2.as_double();
    }
  }

  return *this;
}

var &var::operator*=(const var &other) {
  if (likely (type == INTEGER_TYPE && other.type == INTEGER_TYPE)) {
    as_int() *= other.as_int();
    return *this;
  }

  convert_to_numeric();
  const var arg2 = other.to_numeric();

  if (type == INTEGER_TYPE) {
    if (arg2.type == INTEGER_TYPE) {
      as_int() *= arg2.as_int();
    } else {
      type = FLOAT_TYPE;
      as_double() = as_int() * arg2.as_double();
    }
  } else {
    if (arg2.type == INTEGER_TYPE) {
      as_double() *= arg2.as_int();
    } else {
      as_double() *= arg2.as_double();
    }
  }

  return *this;
}

var &var::operator/=(const var &other) {
  if (likely (type == INTEGER_TYPE && other.type == INTEGER_TYPE)) {
    if (as_int() % other.as_int() == 0) {
      as_int() /= other.as_int();
    } else {
      type = FLOAT_TYPE;
      as_double() = (double)as_int() / other.as_int();
    }
    return *this;
  }

  convert_to_numeric();
  const var arg2 = other.to_numeric();

  if (arg2.type == INTEGER_TYPE) {
    if (arg2.as_int() == 0) {
      php_warning("Integer division by zero");
      type = BOOLEAN_TYPE;
      as_bool() = false;
      return *this;
    }

    if (type == INTEGER_TYPE) {
      if (as_int() % arg2.as_int() == 0) {
        as_int() /= arg2.as_int();
      } else {
        type = FLOAT_TYPE;
        as_double() = (double)as_int() / other.as_int();
      }
    } else {
      as_double() /= arg2.as_int();
    }
  } else {
    if (arg2.as_double() == 0) {
      php_warning("Float division by zero");
      type = BOOLEAN_TYPE;
      as_bool() = false;
      return *this;
    }

    if (type == INTEGER_TYPE) {
      type = FLOAT_TYPE;
      as_double() = as_int() / arg2.as_double();
    } else {
      as_double() /= arg2.as_double();
    }
  }

  return *this;
}

var &var::operator%=(const var &other) {
  int div = other.to_int();
  if (div == 0) {
    php_warning("Modulo by zero");
    *this = false;
    return *this;
  }
  convert_to_int();
  as_int() %= div;

  return *this;
}


var &var::operator&=(const var &other) {
  convert_to_int();
  as_int() &= other.to_int();
  return *this;
}

var &var::operator|=(const var &other) {
  convert_to_int();
  as_int() |= other.to_int();
  return *this;
}

var &var::operator^=(const var &other) {
  convert_to_int();
  as_int() ^= other.to_int();
  return *this;
}

var &var::operator<<=(const var &other) {
  convert_to_int();
  as_int() <<= other.to_int();
  return *this;
}

var &var::operator>>=(const var &other) {
  convert_to_int();
  as_int() >>= other.to_int();
  return *this;
}


var &var::operator++() {
  switch (type) {
    case NULL_TYPE:
      type = INTEGER_TYPE;
      as_int() = 1;
      return *this;
    case BOOLEAN_TYPE:
      php_warning("Can't apply operator ++ to boolean");
      return *this;
    case INTEGER_TYPE:
      ++as_int();
      return *this;
    case FLOAT_TYPE:
      as_double() += 1;
      return *this;
    case STRING_TYPE:
      *this = as_string().to_numeric();
      return ++(*this);
    case ARRAY_TYPE:
      php_warning("Can't apply operator ++ to array");
      return *this;
    default:
      __builtin_unreachable();
  }
}

const var var::operator++(int) {
  switch (type) {
    case NULL_TYPE:
      type = INTEGER_TYPE;
      as_int() = 1;
      return var();
    case BOOLEAN_TYPE:
      php_warning("Can't apply operator ++ to boolean");
      return as_bool();
    case INTEGER_TYPE: {
      var res(as_int());
      ++as_int();
      return res;
    }
    case FLOAT_TYPE: {
      var res(as_double());
      as_double() += 1;
      return res;
    }
    case STRING_TYPE: {
      var res(as_string());
      *this = as_string().to_numeric();
      (*this)++;
      return res;
    }
    case ARRAY_TYPE:
      php_warning("Can't apply operator ++ to array");
      return as_array();
    default:
      __builtin_unreachable();
  }
}

var &var::operator--() {
  if (likely (type == INTEGER_TYPE)) {
    --as_int();
    return *this;
  }

  switch (type) {
    case NULL_TYPE:
      php_warning("Can't apply operator -- to null");
      return *this;
    case BOOLEAN_TYPE:
      php_warning("Can't apply operator -- to boolean");
      return *this;
    case INTEGER_TYPE:
      --as_int();
      return *this;
    case FLOAT_TYPE:
      as_double() -= 1;
      return *this;
    case STRING_TYPE:
      *this = as_string().to_numeric();
      return --(*this);
    case ARRAY_TYPE:
      php_warning("Can't apply operator -- to array");
      return *this;
    default:
      __builtin_unreachable();
  }
}

const var var::operator--(int) {
  if (likely (type == INTEGER_TYPE)) {
    var res(as_int());
    --as_int();
    return res;
  }

  switch (type) {
    case NULL_TYPE:
      php_warning("Can't apply operator -- to null");
      return var();
    case BOOLEAN_TYPE:
      php_warning("Can't apply operator -- to boolean");
      return as_bool();
    case INTEGER_TYPE: {
      var res(as_int());
      --as_int();
      return res;
    }
    case FLOAT_TYPE: {
      var res(as_double());
      as_double() -= 1;
      return res;
    }
    case STRING_TYPE: {
      var res(as_string());
      *this = as_string().to_numeric();
      (*this)--;
      return res;
    }
    case ARRAY_TYPE:
      php_warning("Can't apply operator -- to array");
      return as_array();
    default:
      __builtin_unreachable();
  }
}


bool var::operator!() const {
  return !to_bool();
}


var &var::append(const string &v) {
  if (unlikely (type != STRING_TYPE)) {
    convert_to_string();
  }
  as_string().append(v);
  return *this;
}


void var::destroy() {
  switch (type) {
    case STRING_TYPE:
      as_string().~string();
      break;
    case ARRAY_TYPE:
      as_array().~array<var>();
      break;
    default: {
    }
  }
}

var::~var() {
  // do not remove copy-paste from clear.
  // It makes stacktraces unreadable
  destroy();
  type = NULL_TYPE;
}


void var::clear() {
  destroy();
  type = NULL_TYPE;
}


const var var::to_numeric() const {
  switch (type) {
    case NULL_TYPE:
      return 0;
    case BOOLEAN_TYPE:
      return (as_bool() ? 1 : 0);
    case INTEGER_TYPE:
      return as_int();
    case FLOAT_TYPE:
      return as_double();
    case STRING_TYPE:
      return as_string().to_numeric();
    case ARRAY_TYPE:
      php_warning("Wrong convertion from array to number");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}


bool var::to_bool() const {
  switch (type) {
    case NULL_TYPE:
      return false;
    case BOOLEAN_TYPE:
      return as_bool();
    case INTEGER_TYPE:
      return (bool)as_int();
    case FLOAT_TYPE:
      return (bool)as_double();
    case STRING_TYPE:
      return as_string().to_bool();
    case ARRAY_TYPE:
      return !as_array().empty();
    default:
      __builtin_unreachable();
  }
}

int var::to_int() const {
  switch (type) {
    case NULL_TYPE:
      return 0;
    case BOOLEAN_TYPE:
      return (int)as_bool();
    case INTEGER_TYPE:
      return as_int();
    case FLOAT_TYPE:
      return (int)as_double();
    case STRING_TYPE:
      return as_string().to_int();
    case ARRAY_TYPE:
      php_warning("Wrong convertion from array to int");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}

double var::to_float() const {
  switch (type) {
    case NULL_TYPE:
      return 0.0;
    case BOOLEAN_TYPE:
      return (as_bool() ? 1.0 : 0.0);
    case INTEGER_TYPE:
      return (double)as_int();
    case FLOAT_TYPE:
      return as_double();
    case STRING_TYPE:
      return as_string().to_float();
    case ARRAY_TYPE:
      php_warning("Wrong convertion from array to float");
      return as_array().to_float();
    default:
      __builtin_unreachable();
  }
}

const string var::to_string() const {
  switch (type) {
    case NULL_TYPE:
      return string();
    case BOOLEAN_TYPE:
      return (as_bool() ? string("1", 1) : string());
    case INTEGER_TYPE:
      return string(as_int());
    case FLOAT_TYPE:
      return string(as_double());
    case STRING_TYPE:
      return as_string();
    case ARRAY_TYPE:
      php_warning("Convertion from array to string");
      return string("Array", 5);
    default:
      __builtin_unreachable();
  }
}

const array<var> var::to_array() const {
  switch (type) {
    case NULL_TYPE:
      return array<var>();
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case FLOAT_TYPE:
    case STRING_TYPE: {
      array<var> res(array_size(1, 0, true));
      res.push_back(*this);
      return res;
    }
    case ARRAY_TYPE:
      return as_array();
    default:
      __builtin_unreachable();
  }
}

bool &var::as_bool() { return *reinterpret_cast<bool *>(&storage); }
const bool &var::as_bool() const { return *reinterpret_cast<const bool *>(&storage); }

int &var::as_int() { return *reinterpret_cast<int *>(&storage); }
const int &var::as_int() const { return *reinterpret_cast<const int *>(&storage); }

double &var::as_double() { return *reinterpret_cast<double *>(&storage); }
const double &var::as_double() const { return *reinterpret_cast<const double *>(&storage); }

string &var::as_string() { return *reinterpret_cast<string *>(&storage); }
const string &var::as_string() const { return *reinterpret_cast<const string *>(&storage); }

array<var> &var::as_array() { return *reinterpret_cast<array<var> *>(&storage); }
const array<var> &var::as_array() const { return *reinterpret_cast<const array<var> *>(&storage); }


int var::safe_to_int() const {
  switch (type) {
    case NULL_TYPE:
      return 0;
    case BOOLEAN_TYPE:
      return (int)as_bool();
    case INTEGER_TYPE:
      return as_int();
    case FLOAT_TYPE:
      if (fabs(as_double()) > 2147483648) {
        php_warning("Wrong convertion from double %.6lf to int", as_double());
      }
      return (int)as_double();
    case STRING_TYPE:
      return as_string().safe_to_int();
    case ARRAY_TYPE:
      php_warning("Wrong convertion from array to int");
      return as_array().to_int();
    default:
      __builtin_unreachable();
  }
}


void var::convert_to_numeric() {
  switch (type) {
    case NULL_TYPE:
      type = INTEGER_TYPE;
      as_int() = 0;
      return;
    case BOOLEAN_TYPE:
      type = INTEGER_TYPE;
      as_int() = as_bool();
      return;
    case INTEGER_TYPE:
    case FLOAT_TYPE:
      return;
    case STRING_TYPE:
      *this = as_string().to_numeric();
      return;
    case ARRAY_TYPE: {
      php_warning("Wrong convertion from array to number");
      const int int_val = as_array().to_int();
      as_array().~array<var>();
      type = INTEGER_TYPE;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void var::convert_to_bool() {
  switch (type) {
    case NULL_TYPE:
      type = BOOLEAN_TYPE;
      as_bool() = 0;
      return;
    case BOOLEAN_TYPE:
      return;
    case INTEGER_TYPE:
      type = BOOLEAN_TYPE;
      as_bool() = (bool)as_int();
      return;
    case FLOAT_TYPE:
      type = BOOLEAN_TYPE;
      as_bool() = (bool)as_double();
      return;
    case STRING_TYPE: {
      const bool bool_val = as_string().to_bool();
      as_string().~string();
      type = BOOLEAN_TYPE;
      as_bool() = bool_val;
      return;
    }
    case ARRAY_TYPE: {
      const bool bool_val = as_array().to_bool();
      as_array().~array<var>();
      type = BOOLEAN_TYPE;
      as_bool() = bool_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void var::convert_to_int() {
  switch (type) {
    case NULL_TYPE:
      type = INTEGER_TYPE;
      as_int() = 0;
      return;
    case BOOLEAN_TYPE:
      type = INTEGER_TYPE;
      as_int() = as_bool();
      return;
    case INTEGER_TYPE:
      return;
    case FLOAT_TYPE:
      type = INTEGER_TYPE;
      as_int() = (int)as_double();
      return;
    case STRING_TYPE: {
      const int int_val = as_string().to_int();
      as_string().~string();
      type = INTEGER_TYPE;
      as_int() = int_val;
      return;
    }
    case ARRAY_TYPE: {
      php_warning("Wrong convertion from array to int");
      const int int_val = as_array().to_int();
      as_array().~array<var>();
      type = INTEGER_TYPE;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void var::convert_to_float() {
  switch (type) {
    case NULL_TYPE:
      type = FLOAT_TYPE;
      as_double() = 0.0;
      return;
    case BOOLEAN_TYPE:
      type = FLOAT_TYPE;
      as_double() = as_bool();
      return;
    case INTEGER_TYPE:
      type = FLOAT_TYPE;
      as_double() = (double)as_int();
      return;
    case FLOAT_TYPE:
      return;
    case STRING_TYPE: {
      const double float_val = as_string().to_float();
      as_string().~string();
      type = FLOAT_TYPE;
      as_double() = float_val;
      return;
    }
    case ARRAY_TYPE: {
      php_warning("Wrong convertion from array to float");
      const double float_val = as_array().to_float();
      as_array().~array<var>();
      type = FLOAT_TYPE;
      as_double() = float_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}

void var::convert_to_string() {
  switch (type) {
    case NULL_TYPE:
      type = STRING_TYPE;
      new(&as_string()) string();
      return;
    case BOOLEAN_TYPE:
      type = STRING_TYPE;
      if (as_bool()) {
        new(&as_string()) string("1", 1);
      } else {
        new(&as_string()) string();
      }
      return;
    case INTEGER_TYPE:
      type = STRING_TYPE;
      new(&as_string()) string(as_int());
      return;
    case FLOAT_TYPE:
      type = STRING_TYPE;
      new(&as_string()) string(as_double());
      return;
    case STRING_TYPE:
      return;
    case ARRAY_TYPE:
      php_warning("Converting from array to string");
      as_array().~array<var>();
      type = STRING_TYPE;
      new(&as_string()) string("Array", 5);
      return;
    default:
      __builtin_unreachable();
  }
}


void var::safe_convert_to_int() {
  switch (type) {
    case NULL_TYPE:
      type = INTEGER_TYPE;
      as_int() = 0;
      return;
    case BOOLEAN_TYPE:
      type = INTEGER_TYPE;
      as_int() = (int)as_bool();
      return;
    case INTEGER_TYPE:
      return;
    case FLOAT_TYPE:
      type = INTEGER_TYPE;
      if (fabs(as_double()) > 2147483648) {
        php_warning("Wrong convertion from double %.6lf to int", as_double());
      }
      as_int() = (int)as_double();
      return;
    case STRING_TYPE: {
      const int int_val = as_string().safe_to_int();
      as_string().~string();
      type = INTEGER_TYPE;
      as_int() = int_val;
      return;
    }
    case ARRAY_TYPE: {
      php_warning("Wrong convertion from array to int");
      const int int_val = as_array().to_int();
      as_array().~array<var>();
      type = INTEGER_TYPE;
      as_int() = int_val;
      return;
    }
    default:
      __builtin_unreachable();
  }
}


const bool &var::as_bool(const char *function) const {
  switch (type) {
    case BOOLEAN_TYPE:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_c_str());
      empty_bool = false;
      return empty_bool;
  }
}

const int &var::as_int(const char *function) const {
  switch (type) {
    case INTEGER_TYPE:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_c_str());
      empty_int = 0;
      return empty_int;
  }
}

const double &var::as_float(const char *function) const {
  switch (type) {
    case FLOAT_TYPE:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_c_str());
      empty_float = 0;
      return empty_float;
  }
}

const string &var::as_string(const char *function) const {
  switch (type) {
    case STRING_TYPE:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_c_str());
      empty_string = string();
      return empty_string;
  }
}

const array<var> &var::as_array(const char *function) const {
  switch (type) {
    case ARRAY_TYPE:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      empty_array_var = array<var>();
      return empty_array_var;
  }
}


bool &var::as_bool(const char *function) {
  switch (type) {
    case NULL_TYPE:
      convert_to_bool();
    case BOOLEAN_TYPE:
      return as_bool();
    default:
      php_warning("%s() expects parameter to be boolean, %s is given", function, get_type_c_str());
      empty_bool = false;
      return empty_bool;
  }
}

int &var::as_int(const char *function) {
  switch (type) {
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case FLOAT_TYPE:
    case STRING_TYPE:
      convert_to_int();
    case INTEGER_TYPE:
      return as_int();
    default:
      php_warning("%s() expects parameter to be int, %s is given", function, get_type_c_str());
      empty_int = 0;
      return empty_int;
  }
}

double &var::as_float(const char *function) {
  switch (type) {
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case STRING_TYPE:
      convert_to_float();
    case FLOAT_TYPE:
      return as_double();
    default:
      php_warning("%s() expects parameter to be float, %s is given", function, get_type_c_str());
      empty_float = 0;
      return empty_float;
  }
}

string &var::as_string(const char *function) {
  switch (type) {
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case FLOAT_TYPE:
      convert_to_string();
    case STRING_TYPE:
      return as_string();
    default:
      php_warning("%s() expects parameter to be string, %s is given", function, get_type_c_str());
      empty_string = string();
      return empty_string;
  }
}

array<var> &var::as_array(const char *function) {
  switch (type) {
    case ARRAY_TYPE:
      return as_array();
    default:
      php_warning("%s() expects parameter to be array, %s is given", function, get_type_c_str());
      empty_array_var = array<var>();
      return empty_array_var;
  }
}


bool var::is_numeric() const {
  switch (type) {
    case INTEGER_TYPE:
    case FLOAT_TYPE:
      return true;
    case STRING_TYPE:
      return as_string().is_numeric();
    default:
      return false;
  }
}

bool var::is_scalar() const {
  return type != NULL_TYPE && type != ARRAY_TYPE;
}


bool var::is_null() const {
  return type == NULL_TYPE;
}

bool var::is_bool() const {
  return type == BOOLEAN_TYPE;
}

bool var::is_int() const {
  return type == INTEGER_TYPE;
}

bool var::is_float() const {
  return type == FLOAT_TYPE;
}

bool var::is_string() const {
  return type == STRING_TYPE;
}

bool var::is_array() const {
  return type == ARRAY_TYPE;
}


inline const char *var::get_type_c_str() const {
  switch (type) {
    case NULL_TYPE:
      return "NULL";
    case BOOLEAN_TYPE:
      return "boolean";
    case INTEGER_TYPE:
      return "integer";
    case FLOAT_TYPE:
      return "double";
    case STRING_TYPE:
      return "string";
    case ARRAY_TYPE:
      return "array";
    default:
      __builtin_unreachable();
  }
}

inline const string var::get_type() const {
  const char *result = get_type_c_str();
  return string(result, (dl::size_type)strlen(result));
}


bool var::empty() const {
  return !to_bool();
}

int var::count() const {
  switch (type) {
    case NULL_TYPE:
      return 0;
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case FLOAT_TYPE:
    case STRING_TYPE:
      return 1;
    case ARRAY_TYPE:
      return as_array().count();
    default:
      __builtin_unreachable();
  }
}


void var::swap(var &other) {
  ::swap(type, other.type);
  ::swap(as_double(), other.as_double());
}


var &var::operator[](int int_key) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      php_warning("Writing to string by offset is't supported");
      empty_var = var();
      return empty_var;
    }

    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %d", to_string().c_str(), get_type_c_str(), int_key);

      empty_var = var();
      return empty_var;
    }
  }
  return as_array()[int_key];
}

var &var::operator[](const string &string_key) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      php_warning("Writing to string by offset is't supported");
      empty_var = var();
      return empty_var;
    }

    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());

      empty_var = var();
      return empty_var;
    }
  }

  return as_array()[string_key];
}

var &var::operator[](const var &v) {
  switch (v.type) {
    case NULL_TYPE:
      return (*this)[string()];
    case BOOLEAN_TYPE:
      return (*this)[v.as_bool()];
    case INTEGER_TYPE:
      return (*this)[v.as_int()];
    case FLOAT_TYPE:
      return (*this)[(int)v.as_double()];
    case STRING_TYPE:
      return (*this)[v.as_string()];
    case ARRAY_TYPE:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return (*this)[v.as_array().to_int()];
    default:
      __builtin_unreachable();
  }
}

var &var::operator[](const array<var>::const_iterator &it) {
  return as_array()[it];
}

var &var::operator[](const array<var>::iterator &it) {
  return as_array()[it];
}


void var::set_value(int int_key, const var &v) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      const char c = (v.to_string())[0];

      if (int_key >= 0) {
        const int l = as_string().size();
        if (int_key >= l) {
          as_string().append(int_key + 1 - l, ' ');
        } else {
          as_string().make_not_shared();
        }

        as_string()[int_key] = c;
      } else {
        php_warning("%d is illegal offset for string", int_key);
      }
      return;
    }

    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %d", to_string().c_str(), get_type_c_str(), int_key);
      return;
    }
  }
  return as_array().set_value(int_key, v);
}

void var::set_value(const string &string_key, const var &v) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      int int_val;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        int_val = string_key.to_int();
      }
      if (int_val < 0) {
        return;
      }

      char c = (v.to_string())[0];

      const int l = as_string().size();
      if (int_val >= l) {
        as_string().append(int_val + 1 - l, ' ');
      } else {
        as_string().make_not_shared();
      }

      as_string()[int_val] = c;
      return;
    }

    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
      return;
    }
  }

  return as_array().set_value(string_key, v);
}

void var::set_value(const string &string_key, const var &v, int precomuted_hash) {
  return type == ARRAY_TYPE ? as_array().set_value(string_key, v, precomuted_hash) : set_value(string_key, v);
}

void var::set_value(const var &v, const var &value) {
  switch (v.type) {
    case NULL_TYPE:
      return set_value(string(), value);
    case BOOLEAN_TYPE:
      return set_value(v.as_bool(), value);
    case INTEGER_TYPE:
      return set_value(v.as_int(), value);
    case FLOAT_TYPE:
      return set_value((int)v.as_double(), value);
    case STRING_TYPE:
      return set_value(v.as_string(), value);
    case ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return;
    default:
      __builtin_unreachable();
  }
}

void var::set_value(const array<var>::const_iterator &it) {
  return as_array().set_value(it);
}

void var::set_value(const array<var>::iterator &it) {
  return as_array().set_value(it);
}


const var var::get_value(int int_key) const {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      if ((dl::size_type)int_key >= as_string().size()) {
        return string();
      }
      return string(1, as_string()[int_key]);
    }

    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %d", to_string().c_str(), get_type_c_str(), int_key);
    }
    return var();
  }

  return as_array().get_value(int_key);
}

const var var::get_value(const string &string_key) const {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      int int_val;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        int_val = string_key.to_int();
      }
      if ((dl::size_type)int_val >= as_string().size()) {
        return string();
      }
      return string(1, as_string()[int_val]);
    }

    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use a value \"%s\" of type %s as an array, index = %s", to_string().c_str(), get_type_c_str(), string_key.c_str());
    }
    return var();
  }

  return as_array().get_value(string_key);
}

const var var::get_value(const string &string_key, int precomuted_hash) const {
  return type == ARRAY_TYPE ? as_array().get_value(string_key, precomuted_hash) : get_value(string_key);
}

const var var::get_value(const var &v) const {
  switch (v.type) {
    case NULL_TYPE:
      return get_value(string());
    case BOOLEAN_TYPE:
      return get_value(v.as_bool());
    case INTEGER_TYPE:
      return get_value(v.as_int());
    case FLOAT_TYPE:
      return get_value((int)v.as_double());
    case STRING_TYPE:
      return get_value(v.as_string());
    case ARRAY_TYPE:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return var();
    default:
      __builtin_unreachable();
  }
}

const var var::get_value(const array<var>::const_iterator &it) const {
  return as_array().get_value(it);
}

const var var::get_value(const array<var>::iterator &it) const {
  return as_array().get_value(it);
}


void var::push_back(const var &v) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      return;
    }
  }

  return as_array().push_back(v);
}

const var var::push_back_return(const var &v) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == NULL_TYPE || (type == BOOLEAN_TYPE && !as_bool())) {
      type = ARRAY_TYPE;
      new(&as_array()) array<var>();
    } else {
      php_warning("[] operator not supported for type %s", get_type_c_str());
      empty_var = var();
      return empty_var;
    }
  }

  return as_array().push_back_return(v);
}


bool var::isset(int int_key) const {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      return (dl::size_type)int_key < as_string().size();
    }

    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in isset", get_type_c_str());
    }
    return false;
  }

  return as_array().isset(int_key);
}

bool var::isset(const string &string_key) const {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      int int_val;
      if (!string_key.try_to_int(&int_val)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        return false;
      }
      return (dl::size_type)int_val < as_string().size();
    }

    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in isset", get_type_c_str());
    }
    return false;
  }

  return as_array().isset(string_key);
}

bool var::isset(const var &v) const {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type == STRING_TYPE) {
      return (dl::size_type)(v.to_int()) < as_string().size();
    }

    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in isset", get_type_c_str());
    }
    return false;
  }

  switch (v.type) {
    case NULL_TYPE:
      return as_array().isset(string());
    case BOOLEAN_TYPE:
      return as_array().isset(v.as_bool());
    case INTEGER_TYPE:
      return as_array().isset(v.as_int());
    case FLOAT_TYPE:
      return as_array().isset((int)v.as_double());
    case STRING_TYPE:
      return as_array().isset(v.as_string());
    case ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return false;
    default:
      __builtin_unreachable();
  }
}

bool var::isset(const array<var>::const_iterator &it) const {
  return as_array().isset(it);
}

bool var::isset(const array<var>::iterator &it) const {
  return as_array().isset(it);
}


void var::unset(int int_key) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  return as_array().unset(int_key);
}

void var::unset(const string &string_key) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  return as_array().unset(string_key);
}

void var::unset(const var &v) {
  if (unlikely (type != ARRAY_TYPE)) {
    if (type != NULL_TYPE && (type != BOOLEAN_TYPE || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_c_str());
    }
    return;
  }

  switch (v.type) {
    case NULL_TYPE:
      return as_array().unset(string());
    case BOOLEAN_TYPE:
      return as_array().unset(v.as_bool());
    case INTEGER_TYPE:
      return as_array().unset(v.as_int());
    case FLOAT_TYPE:
      return as_array().unset((int)v.as_double());
    case STRING_TYPE:
      return as_array().unset(v.as_string());
    case ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return;
    default:
      __builtin_unreachable();
  }
}

void var::unset(const array<var>::const_iterator &it) {
  return as_array().unset(it);
}

void var::unset(const array<var>::iterator &it) {
  return as_array().unset(it);
}


array<var>::const_iterator var::begin() const {
  if (likely (type == ARRAY_TYPE)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s \"%s\" is given", get_type_c_str(), to_string().c_str());
  return array<var>::const_iterator();
}

array<var>::const_iterator var::end() const {
  if (likely (type == ARRAY_TYPE)) {
    return as_array().end();
  }
  return array<var>::const_iterator();
}


array<var>::iterator var::begin() {
  if (likely (type == ARRAY_TYPE)) {
    return as_array().begin();
  }
  php_warning("Invalid argument supplied for foreach(), %s \"%s\" is given", get_type_c_str(), to_string().c_str());
  return array<var>::iterator();
}

array<var>::iterator var::end() {
  if (likely (type == ARRAY_TYPE)) {
    return as_array().end();
  }
  return array<var>::iterator();
}


int var::get_reference_counter() const {
  switch (type) {
    case NULL_TYPE:
      return -1;
    case BOOLEAN_TYPE:
      return -2;
    case INTEGER_TYPE:
      return -3;
    case FLOAT_TYPE:
      return -4;
    case STRING_TYPE:
      return as_string().get_reference_counter();
    case ARRAY_TYPE:
      return as_array().get_reference_counter();
    default:
      __builtin_unreachable();
  }
}

void var::set_reference_counter_to_const() {
  switch (type) {
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case FLOAT_TYPE:
      return;
    case STRING_TYPE:
      return as_string().set_reference_counter_to_const();
    case ARRAY_TYPE:
      return as_array().set_reference_counter_to_const();
    default:
      __builtin_unreachable();
  }
}

inline dl::size_type var::estimate_memory_usage() const {
  switch (type) {
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
    case FLOAT_TYPE:
      return 0;
    case STRING_TYPE:
      return as_string().estimate_memory_usage();
    case ARRAY_TYPE:
      return as_array().estimate_memory_usage();
    default:
      __builtin_unreachable();
  }
}

const var operator+(const var &lhs, const var &rhs) {
  if (likely (lhs.type == var::INTEGER_TYPE && rhs.type == var::INTEGER_TYPE)) {
    return lhs.as_int() + rhs.as_int();
  }

  if (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE) {
    return lhs.as_array() + rhs.as_array();
  }

  const var arg1 = lhs.to_numeric();
  const var arg2 = rhs.to_numeric();

  if (arg1.type == var::INTEGER_TYPE) {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_int() + arg2.as_int();
    } else {
      return arg1.as_int() + arg2.as_double();
    }
  } else {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_double() + arg2.as_int();
    } else {
      return arg1.as_double() + arg2.as_double();
    }
  }
}

const var operator-(const var &lhs, const var &rhs) {
  if (likely (lhs.type == var::INTEGER_TYPE && rhs.type == var::INTEGER_TYPE)) {
    return lhs.as_int() - rhs.as_int();
  }

  const var arg1 = lhs.to_numeric();
  const var arg2 = rhs.to_numeric();

  if (arg1.type == var::INTEGER_TYPE) {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_int() - arg2.as_int();
    } else {
      return arg1.as_int() - arg2.as_double();
    }
  } else {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_double() - arg2.as_int();
    } else {
      return arg1.as_double() - arg2.as_double();
    }
  }
}

const var operator*(const var &lhs, const var &rhs) {
  if (likely (lhs.type == var::INTEGER_TYPE && rhs.type == var::INTEGER_TYPE)) {
    return lhs.as_int() * rhs.as_int();
  }

  const var arg1 = lhs.to_numeric();
  const var arg2 = rhs.to_numeric();

  if (arg1.type == var::INTEGER_TYPE) {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_int() * arg2.as_int();
    } else {
      return arg1.as_int() * arg2.as_double();
    }
  } else {
    if (arg2.type == var::INTEGER_TYPE) {
      return arg1.as_double() * arg2.as_int();
    } else {
      return arg1.as_double() * arg2.as_double();
    }
  }
}

const var operator/(const var &lhs, const var &rhs) {
  if (likely (lhs.type == var::INTEGER_TYPE && rhs.type == var::INTEGER_TYPE)) {
    if (rhs.as_int() == 0) {
      php_warning("Integer division by zero");
      return false;
    }

    if (lhs.as_int() % rhs.as_int() == 0) {
      return lhs.as_int() / rhs.as_int();
    }
    return (double)lhs.as_int() / rhs.as_int();
  }

  const var arg1 = lhs.to_numeric();
  const var arg2 = rhs.to_numeric();

  if (arg2.type == var::INTEGER_TYPE) {
    if (arg2.as_int() == 0) {
      php_warning("Integer division by zero");
      return false;
    }

    if (arg1.type == var::INTEGER_TYPE) {
      if (arg1.as_int() % arg2.as_int() == 0) {
        return arg1.as_int() / arg2.as_int();
      }
      return (double)arg1.as_int() / arg2.as_int();
    } else {
      return arg1.as_double() / arg2.as_int();
    }
  } else {
    if (arg2.as_double() == 0.0) {
      php_warning("Float division by zero");
      return false;
    }

    if (arg1.type == var::INTEGER_TYPE) {
      return arg1.as_int() / arg2.as_double();
    } else {
      return arg1.as_double() / arg2.as_double();
    }
  }
}

const var operator%(const var &lhs, const var &rhs) {
  int div = rhs.to_int();
  if (div == 0) {
    php_warning("Modulo by zero");
    return false;
  }
  return lhs.to_int() % div;
}


const var operator-(const string &lhs) {
  var arg1 = lhs.to_numeric();

  if (arg1.type == var::INTEGER_TYPE) {
    arg1.as_int() = -arg1.as_int();
  } else {
    arg1.as_double() = -arg1.as_double();
  }
  return arg1;
}

const var operator+(const string &lhs) {
  return lhs.to_numeric();
}


int operator&(const var &lhs, const var &rhs) {
  return lhs.to_int() & rhs.to_int();
}

int operator|(const var &lhs, const var &rhs) {
  return lhs.to_int() | rhs.to_int();
}

int operator^(const var &lhs, const var &rhs) {
  return lhs.to_int() ^ rhs.to_int();
}

int operator<<(const var &lhs, const var &rhs) {
  return lhs.to_int() << rhs.to_int();
}

int operator>>(const var &lhs, const var &rhs) {
  return lhs.to_int() >> rhs.to_int();
}


bool eq2(const var &lhs, const var &rhs) {
  if (unlikely (lhs.type == var::STRING_TYPE)) {
    if (likely (rhs.type == var::STRING_TYPE)) {
      return eq2(lhs.as_string(), rhs.as_string());
    } else if (unlikely (rhs.type == var::NULL_TYPE)) {
      return lhs.as_string().size() == 0;
    }
  } else if (unlikely (rhs.type == var::STRING_TYPE)) {
    if (unlikely (lhs.type == var::NULL_TYPE)) {
      return rhs.as_string().size() == 0;
    }
  }
  if (lhs.type == var::BOOLEAN_TYPE || rhs.type == var::BOOLEAN_TYPE || lhs.type == var::NULL_TYPE || rhs.type == var::NULL_TYPE) {
    return lhs.to_bool() == rhs.to_bool();
  }

  if (unlikely (lhs.type == var::ARRAY_TYPE || rhs.type == var::ARRAY_TYPE)) {
    if (likely (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE)) {
      return eq2(lhs.as_array(), rhs.as_array());
    }

    php_warning("Unsupported operand types for operator == (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return false;
  }

  return lhs.to_float() == rhs.to_float();
}

bool neq2(const var &lhs, const var &rhs) {
  return !eq2(lhs, rhs);
}

bool operator<=(const var &lhs, const var &rhs) {
  if (unlikely (lhs.type == var::STRING_TYPE)) {
    if (likely (rhs.type == var::STRING_TYPE)) {
      return compare_strings_php_order(lhs.as_string(), rhs.as_string()) <= 0;
    } else if (unlikely (rhs.type == var::NULL_TYPE)) {
      return lhs.as_string().size() == 0;
    }
  } else if (unlikely (rhs.type == var::STRING_TYPE)) {
    if (unlikely (lhs.type == var::NULL_TYPE)) {
      return true;
    }
  }
  if (lhs.type == var::BOOLEAN_TYPE || rhs.type == var::BOOLEAN_TYPE || lhs.type == var::NULL_TYPE || rhs.type == var::NULL_TYPE) {
    return lhs.to_bool() <= rhs.to_bool();
  }

  if (unlikely (lhs.type == var::ARRAY_TYPE || rhs.type == var::ARRAY_TYPE)) {
    if (likely (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE)) {
      return lhs.as_array().count() <= rhs.as_array().count();
    }

    php_warning("Unsupported operand types for operator <= (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return rhs.type == var::ARRAY_TYPE;
  }

  return lhs.to_float() <= rhs.to_float();
}

bool operator>=(const var &lhs, const var &rhs) {
  if (unlikely (lhs.type == var::STRING_TYPE)) {
    if (likely (rhs.type == var::STRING_TYPE)) {
      return compare_strings_php_order(lhs.as_string(), rhs.as_string()) >= 0;
    } else if (unlikely (rhs.type == var::NULL_TYPE)) {
      return true;
    }
  } else if (unlikely (rhs.type == var::STRING_TYPE)) {
    if (unlikely (lhs.type == var::NULL_TYPE)) {
      return rhs.as_string().size() == 0;
    }
  }
  if (lhs.type == var::BOOLEAN_TYPE || rhs.type == var::BOOLEAN_TYPE || lhs.type == var::NULL_TYPE || rhs.type == var::NULL_TYPE) {
    return lhs.to_bool() >= rhs.to_bool();
  }

  if (unlikely (lhs.type == var::ARRAY_TYPE || rhs.type == var::ARRAY_TYPE)) {
    if (likely (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE)) {
      return lhs.as_array().count() >= rhs.as_array().count();
    }

    php_warning("Unsupported operand types for operator >= (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return lhs.type == var::ARRAY_TYPE;
  }

  return lhs.to_float() >= rhs.to_float();
}

bool operator<(const var &lhs, const var &rhs) {
  if (unlikely (lhs.type == var::STRING_TYPE)) {
    if (likely (rhs.type == var::STRING_TYPE)) {
      return compare_strings_php_order(lhs.as_string(), rhs.as_string()) < 0;
    } else if (unlikely (rhs.type == var::NULL_TYPE)) {
      return false;
    }
  } else if (unlikely (rhs.type == var::STRING_TYPE)) {
    if (unlikely (lhs.type == var::NULL_TYPE)) {
      return rhs.as_string().size() != 0;
    }
  }
  if (lhs.type == var::BOOLEAN_TYPE || rhs.type == var::BOOLEAN_TYPE || lhs.type == var::NULL_TYPE || rhs.type == var::NULL_TYPE) {
    return lhs.to_bool() < rhs.to_bool();
  }

  if (unlikely (lhs.type == var::ARRAY_TYPE || rhs.type == var::ARRAY_TYPE)) {
    if (likely (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE)) {
      return lhs.as_array().count() < rhs.as_array().count();
    }

    php_warning("Unsupported operand types for operator < (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return lhs.type != var::ARRAY_TYPE;
  }

  return lhs.to_float() < rhs.to_float();
}

bool operator>(const var &lhs, const var &rhs) {
  if (unlikely (lhs.type == var::STRING_TYPE)) {
    if (likely (rhs.type == var::STRING_TYPE)) {
      return compare_strings_php_order(lhs.as_string(), rhs.as_string()) > 0;
    } else if (unlikely (rhs.type == var::NULL_TYPE)) {
      return lhs.as_string().size() != 0;
    }
  } else if (unlikely (rhs.type == var::STRING_TYPE)) {
    if (unlikely (lhs.type == var::NULL_TYPE)) {
      return false;
    }
  }
  if (lhs.type == var::BOOLEAN_TYPE || rhs.type == var::BOOLEAN_TYPE || lhs.type == var::NULL_TYPE || rhs.type == var::NULL_TYPE) {
    return lhs.to_bool() > rhs.to_bool();
  }

  if (unlikely (lhs.type == var::ARRAY_TYPE || rhs.type == var::ARRAY_TYPE)) {
    if (likely (lhs.type == var::ARRAY_TYPE && rhs.type == var::ARRAY_TYPE)) {
      return lhs.as_array().count() > rhs.as_array().count();
    }

    php_warning("Unsupported operand types for operator > (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return rhs.type != var::ARRAY_TYPE;
  }

  return lhs.to_float() > rhs.to_float();
}

bool equals(const var &lhs, const var &rhs) {
  if (lhs.type != rhs.type) {
    return false;
  }

  switch (lhs.type) {
    case var::NULL_TYPE:
      return true;
    case var::BOOLEAN_TYPE:
      return lhs.as_bool() == rhs.as_bool();
    case var::INTEGER_TYPE:
      return lhs.as_int() == rhs.as_int();
    case var::FLOAT_TYPE:
      return lhs.as_double() == rhs.as_double();
    case var::STRING_TYPE:
      return lhs.as_string() == rhs.as_string();
    case var::ARRAY_TYPE:
      return equals(lhs.as_array(), rhs.as_array());
    default:
      __builtin_unreachable();
  }
}


void swap(var &lhs, var &rhs) {
  lhs.swap(rhs);
}


bool eq2(bool lhs, bool rhs) {
  return lhs == rhs;
}

bool eq2(int lhs, int rhs) {
  return lhs == rhs;
}

bool eq2(double lhs, double rhs) {
  return lhs == rhs;
}

bool eq2(bool lhs, int rhs) {
  return lhs != !rhs;
}

bool eq2(bool lhs, double rhs) {
  return lhs != (rhs == 0.0);
}

bool eq2(int lhs, bool rhs) {
  return rhs != !lhs;
}

bool eq2(double lhs, bool rhs) {
  return rhs != (lhs == 0.0);
}

bool eq2(int lhs, double rhs) {
  return lhs == rhs;
}

bool eq2(double lhs, int rhs) {
  return lhs == rhs;
}


bool eq2(bool lhs, const string &rhs) {
  return lhs == rhs.to_bool();
}

bool eq2(int lhs, const string &rhs) {
  return lhs == rhs.to_float();
}

bool eq2(double lhs, const string &rhs) {
  return lhs == rhs.to_float();
}

bool eq2(const string &lhs, bool rhs) {
  return rhs == lhs.to_bool();
}

bool eq2(const string &lhs, int rhs) {
  return rhs == lhs.to_float();
}

bool eq2(const string &lhs, double rhs) {
  return rhs == lhs.to_float();
}

template<class T>
bool eq2(bool lhs, const array<T> &rhs) {
  return lhs == !rhs.empty();
}

template<class ...Args>
bool eq2(bool lhs, const std::tuple<Args...> &) {
  return lhs;
}

template<class T>
bool eq2(int, const array<T> &) {
  php_warning("Unsupported operand types for operator == (int and array)");
  return false;
}

template<class T>
bool eq2(double, const array<T> &) {
  php_warning("Unsupported operand types for operator == (float and array)");
  return false;
}

template<class T>
bool eq2(const string &, const array<T> &) {
  php_warning("Unsupported operand types for operator == (string and array)");
  return false;
}

template<class T>
bool eq2(const array<T> &lhs, bool rhs) {
  return rhs == !lhs.empty();
}

template<class ...Args>
bool eq2(const std::tuple<Args...> &, bool rhs) {
  return rhs;
}

template<class T>
bool eq2(const array<T> &, int) {
  php_warning("Unsupported operand types for operator == (array and int)");
  return false;
}

template<class T>
bool eq2(const array<T> &, double) {
  php_warning("Unsupported operand types for operator == (array and float)");
  return false;
}

template<class T>
bool eq2(const array<T> &, const string &) {
  php_warning("Unsupported operand types for operator == (array and string)");
  return false;
}


template<class T>
bool eq2(bool lhs, const class_instance<T> &rhs) {
  return lhs == !rhs.is_null();
}

template<class T>
bool eq2(int lhs, const class_instance<T> &rhs) {
  php_warning("Unsupported operand types for operator == (int and object)");
  return (lhs == 0 && rhs.is_null()) || (lhs == 1 && !rhs.is_null());
}

template<class T>
bool eq2(double lhs, const class_instance<T> &rhs) {
  php_warning("Unsupported operand types for operator == (float and object)");
  return (lhs == 0.0 && rhs.is_null()) || (lhs == 1.0 && !rhs.is_null());
}

template<class T>
bool eq2(const string &lhs, const class_instance<T> &rhs) {
  php_warning("Unsupported operand types for operator == (string and object)");
  return lhs.empty() && rhs.is_null();
}

template<class T, class T1>
bool eq2(const array<T1> &, const class_instance<T> &) {
  php_warning("Unsupported operand types for operator == (array and object)");
  return false;
}

template<class T>
bool eq2(const class_instance<T> &lhs, bool rhs) {
  return rhs == !lhs.is_null();
}

template<class T>
inline bool eq2(const class_instance<T> &lhs, const class_instance<T> &rhs) {
  php_warning("Using == on objects is bad, please use ===");
  return lhs.o == rhs.o;
}

template<class T1, class T2>
bool eq2(const class_instance<T1> &, const class_instance<T2> &) {
  php_warning("Using == on objects is bad, please use ===");
  return false;
}

template<class T>
bool eq2(const class_instance<T> &lhs, int rhs) {
  php_warning("Unsupported operand types for operator == (object and int)");
  return (rhs == 0 && lhs.is_null()) || (rhs == 1 && !lhs.is_null());
}

template<class T>
bool eq2(const class_instance<T> &lhs, double rhs) {
  php_warning("Unsupported operand types for operator == (object and float)");
  return (rhs == 0.0 && lhs.is_null()) || (rhs == 1.0 && !lhs.is_null());
}

template<class T>
bool eq2(const class_instance<T> &lhs, const string &rhs) {
  php_warning("Unsupported operand types for operator == (object and string)");
  return rhs.empty() && lhs.is_null();
}

template<class T, class T1>
bool eq2(const class_instance<T> &, const array<T1> &) {
  php_warning("Unsupported operand types for operator == (object and array)");
  return false;
}


bool eq2(bool lhs, const var &rhs) {
  return lhs == rhs.to_bool();
}

bool eq2(int lhs, const var &rhs) {
  switch (rhs.type) {
    case var::NULL_TYPE:
      return lhs == 0;
    case var::BOOLEAN_TYPE:
      return !!lhs == rhs.as_bool();
    case var::INTEGER_TYPE:
      return lhs == rhs.as_int();
    case var::FLOAT_TYPE:
      return lhs == rhs.as_double();
    case var::STRING_TYPE:
      return lhs == rhs.as_string().to_float();
    case var::ARRAY_TYPE:
      php_warning("Unsupported operand types for operator == (int and array)");
      return false;
    default:
      __builtin_unreachable();
  }
}

bool eq2(double lhs, const var &rhs) {
  switch (rhs.type) {
    case var::NULL_TYPE:
      return lhs == 0.0;
    case var::BOOLEAN_TYPE:
      return (lhs != 0.0) == rhs.as_bool();
    case var::INTEGER_TYPE:
      return lhs == rhs.as_int();
    case var::FLOAT_TYPE:
      return lhs == rhs.as_double();
    case var::STRING_TYPE:
      return lhs == rhs.as_string().to_float();
    case var::ARRAY_TYPE:
      php_warning("Unsupported operand types for operator == (float and array)");
      return false;
    default:
      __builtin_unreachable();
  }
}

bool eq2(const string &lhs, const var &rhs) {
  return eq2(var(lhs), rhs);
}

template<class T>
bool eq2(const array<T> &lhs, const var &rhs) {
  if (likely (rhs.is_array())) {
    return eq2(lhs, rhs.as_array());
  }

  if (rhs.is_bool()) {
    return lhs.empty() != rhs.as_bool();
  }
  if (rhs.is_null()) {
    return lhs.empty();
  }

  php_warning("Unsupported operand types for operator == (array and %s)", rhs.get_type_c_str());
  return false;
}

template<class T>
bool eq2(const class_instance<T> &lhs, const var &rhs) {
  if (rhs.is_null()) {
    return lhs.is_null();
  }

  php_warning("Unsupported operand types for operator == (object and %s)", rhs.get_type_c_str());
  return false;
}

bool eq2(const var &lhs, bool rhs) {
  return rhs == lhs.to_bool();
}

bool eq2(const var &lhs, int rhs) {
  switch (lhs.type) {
    case var::NULL_TYPE:
      return rhs == 0;
    case var::BOOLEAN_TYPE:
      return !!rhs == lhs.as_bool();
    case var::INTEGER_TYPE:
      return rhs == lhs.as_int();
    case var::FLOAT_TYPE:
      return rhs == lhs.as_double();
    case var::STRING_TYPE:
      return rhs == lhs.as_string().to_float();
    case var::ARRAY_TYPE:
      php_warning("Unsupported operand types for operator == (array and int)");
      return false;
    default:
      __builtin_unreachable();
  }
}

bool eq2(const var &lhs, double rhs) {
  switch (lhs.type) {
    case var::NULL_TYPE:
      return rhs == 0.0;
    case var::BOOLEAN_TYPE:
      return (rhs != 0.0) == lhs.as_bool();
    case var::INTEGER_TYPE:
      return rhs == lhs.as_int();
    case var::FLOAT_TYPE:
      return rhs == lhs.as_double();
    case var::STRING_TYPE:
      return rhs == lhs.as_string().to_float();
    case var::ARRAY_TYPE:
      php_warning("Unsupported operand types for operator == (array and float)");
      return false;
    default:
      __builtin_unreachable();
  }
}

bool eq2(const var &lhs, const string &rhs) {
  return eq2(var(rhs), lhs);
}

template<class T>
bool eq2(const var &lhs, const array<T> &rhs) {
  if (likely (lhs.is_array())) {
    return eq2(lhs.as_array(), rhs);
  }

  if (lhs.is_bool()) {
    return rhs.empty() != lhs.as_bool();
  }
  if (lhs.is_null()) {
    return rhs.empty();
  }

  php_warning("Unsupported operand types for operator == (%s and array)", lhs.get_type_c_str());
  return false;
}

template<class T>
bool eq2(const var &lhs, const class_instance<T> &) {
  php_warning("Unsupported operand types for operator == (%s and object)", lhs.get_type_c_str());
  return false;
}


template<class T1, class T2>
bool neq2(const T1 &lhs, const T2 &rhs) {
  return !eq2(lhs, rhs);
}


bool equals(bool lhs, const var &rhs) {
  return rhs.is_bool() && equals(lhs, rhs.as_bool());
}

bool equals(int lhs, const var &rhs) {
  return rhs.is_int() && equals(lhs, rhs.as_int());
}

bool equals(double lhs, const var &rhs) {
  return rhs.is_float() && equals(lhs, rhs.as_double());
}

bool equals(const string &lhs, const var &rhs) {
  return rhs.is_string() && equals(lhs, rhs.as_string());
}

template<class T>
bool equals(const array<T> &lhs, const var &rhs) {
  return rhs.is_array() && equals(lhs, rhs.as_array());
}

template<class T>
bool equals(const class_instance<T> &, const var &) {
  return false;
}

template<class T>
bool equals(const class_instance<T> &lhs, bool rhs) {
  return rhs ? false : lhs.is_null();
}

bool equals(const var &lhs, bool rhs) {
  return lhs.is_bool() && equals(rhs, lhs.as_bool());
}

bool equals(const var &lhs, int rhs) {
  return lhs.is_int() && equals(rhs, lhs.as_int());
}

bool equals(const var &lhs, double rhs) {
  return lhs.is_float() && equals(rhs, lhs.as_double());
}

bool equals(const var &lhs, const string &rhs) {
  return lhs.is_string() && equals(rhs, lhs.as_string());
}

template<class T>
bool equals(const var &lhs, const array<T> &rhs) {
  return lhs.is_array() && equals(rhs, lhs.as_array());
}

template<class T>
bool equals(const class_instance<T> &lhs, const class_instance<T> &rhs) {
  return lhs.o == rhs.o;
}

template<class T1, class T2>
std::enable_if_t<std::is_base_of<T1, T2>{} || std::is_base_of<T2, T1>{}, bool> equals(const class_instance<T1> &lhs, const class_instance<T2> &rhs) {
  return dynamic_cast<void *>(lhs.get()) == dynamic_cast<void *>(rhs.get());
}

template<class T1, class T2>
std::enable_if_t<!std::is_base_of<T1, T2>{} && !std::is_base_of<T2, T1>{}, bool>  equals(const class_instance<T1> &, const class_instance<T2> &) {
  return false;
}

template<class T>
inline bool equals(const var &, const class_instance<T> &) {
  return false;
}

template<class T>
bool equals(bool lhs, const class_instance<T> &rhs) {
  return equals(rhs, lhs);
}


template<class T>
bool eq2(const var &v, const OrFalse<T> &value) {
  return likely (value.bool_value) ? eq2(value.value, v) : eq2(false, v);
}

template<class T>
bool eq2(const OrFalse<T> &value, const var &v) {
  return likely (value.bool_value) ? eq2(value.value, v) : eq2(false, v);
}

template<class T>
bool equals(const OrFalse<T> &value, const var &v) {
  return likely (value.bool_value) ? equals(value.value, v) : equals(false, v);
}

template<class T>
bool equals(const var &v, const OrFalse<T> &value) {
  return likely (value.bool_value) ? equals(value.value, v) : equals(false, v);
}

template<class T>
string_buffer &operator<<(string_buffer &sb, const OrFalse<T> &v) {
  if (!v.bool_value) {
    return sb << false;
  }

  return sb << v.val();
}

string_buffer &operator<<(string_buffer &sb, const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return sb;
    case var::BOOLEAN_TYPE:
      return sb << v.as_bool();
    case var::INTEGER_TYPE:
      return sb << v.as_int();
    case var::FLOAT_TYPE:
      return sb << string(v.as_double());
    case var::STRING_TYPE:
      return sb << v.as_string();
    case var::ARRAY_TYPE:
      php_warning("Convertion from array to string");
      return sb.append("Array", 5);
    default:
      __builtin_unreachable();
  }
}
