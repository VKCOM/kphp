#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<class T, class = enable_for_bool_int_double<T>>
inline bool f$boolval(const T &val) {
  return static_cast<bool>(val);
}

inline bool f$boolval(const string &val) {
  return val.to_bool();
}

template<class T>
inline bool f$boolval(const array<T> &val) {
  return !val.empty();
}

template<class T>
inline bool f$boolval(const class_instance<T> &val) {
  return !val.is_null();
}

template<class ...Args>
inline bool f$boolval(const std::tuple<Args...> &) {
  return true;
}

template<class T>
inline bool f$boolval(const Optional<T> &val) {
  return val.has_value() ? f$boolval(val.val()) : false;
}

inline bool f$boolval(const var &val) {
  return val.to_bool();
}


template<class T, class = enable_for_bool_int_double<T>>
inline int f$intval(const T &val) {
  return static_cast<int>(val);
}

inline int f$intval(const string &val) {
  return val.to_int();
}

template<class T>
inline int f$intval(const array<T> &val) {
  php_warning("Wrong convertion from array to int");
  return val.to_int();
}

template<class T>
inline int f$intval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to int");
  return 1;
}

inline int f$intval(const var &val) {
  return val.to_int();
}

template<class T>
inline int f$intval(const Optional<T> &val) {
  return val.has_value() ? f$intval(val.val()) : 0;
}


inline int f$safe_intval(const bool &val) {
  return val;
}

inline const int &f$safe_intval(const int &val) {
  return val;
}

inline int f$safe_intval(const double &val) {
  if (fabs(val) > 2147483648) {
    php_warning("Wrong convertion from double %.6lf to int", val);
  }
  return (int)val;
}

inline int f$safe_intval(const string &val) {
  return val.safe_to_int();
}

template<class T>
inline int f$safe_intval(const array<T> &val) {
  php_warning("Wrong convertion from array to int");
  return val.to_int();
}

template<class T>
inline int f$safe_intval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to int");
  return 1;
}

inline int f$safe_intval(const var &val) {
  return val.safe_to_int();
}


template<class T, class = enable_for_bool_int_double<T>>
inline double f$floatval(const T &val) {
  return static_cast<double>(val);
}

inline double f$floatval(const string &val) {
  return val.to_float();
}

template<class T>
inline double f$floatval(const array<T> &val) {
  php_warning("Wrong convertion from array to float");
  return val.to_float();
}

template<class T>
inline double f$floatval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to float");
  return 1.0;
}

inline double f$floatval(const var &val) {
  return val.to_float();
}

template<class T>
inline double f$floatval(const Optional<T> &val) {
  return val.has_value() ? f$floatval(val.val()) : 0;
}


inline string f$strval(const bool &val) {
  return (val ? string("1", 1) : string());
}

inline string f$strval(const int &val) {
  return string(val);
}

inline string f$strval(const double &val) {
  return string(val);
}

inline string f$strval(const string &val) {
  return val;
}

inline string &&f$strval(string &&val) {
  return std::move(val);
}

template<class T>
inline string f$strval(const array<T> &) {
  php_warning("Convertion from array to string");
  return string("Array", 5);
}

template<class ...Args>
inline string f$strval(const std::tuple<Args...> &) {
  php_warning("Convertion from tuple to string");
  return string("Array", 5);
}

template<class T>
inline string f$strval(const Optional<T> &val) {
  return val.has_value() ? f$strval(val.val()) : f$strval(false);
}

template<class T>
inline string f$strval(Optional<T> &&val) {
  return val.has_value() ? f$strval(std::move(val.val())) : f$strval(false);
}

template<class T>
inline string f$strval(const class_instance<T> &val) {
  return string(val.get_class());
}

inline string f$strval(const var &val) {
  return val.to_string();
}

inline string f$strval(var &&val) {
  return val.is_string() ? std::move(val.as_string()) : val.to_string();
}

template<class T>
inline array<T> f$arrayval(const T &val) {
  array<T> res(array_size(1, 0, true));
  res.push_back(val);
  return res;
}

template<class T>
inline const array<T> &f$arrayval(const array<T> &val) {
  return val;
}

template<class T>
inline array<T> &&f$arrayval(array<T> &&val) {
  return std::move(val);
}

template<class T>
inline array<var> f$arrayval(const class_instance<T> &) {
  php_warning("Can not convert class instance to array");
  return array<var>();
}

inline array<var> f$arrayval(const var &val) {
  return val.to_array();
}

inline array<var> f$arrayval(var &&val) {
  return val.is_array() ? std::move(val.as_array()) : val.to_array();
}

namespace {

template<typename T>
inline std::enable_if_t<vk::is_type_in_list<T, bool, var>{}, array<T>> false_cast_to_array() {
  return f$arrayval(T{false});
}

template<typename T>
inline std::enable_if_t<!vk::is_type_in_list<T, bool, var>{}, array<T>> false_cast_to_array() {
  php_warning("Dangerous cast false to array, the result will be different from PHP");
  return array<T>{};
}

} // namespace

template<class T>
inline array<T> f$arrayval(const Optional<T> &val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(val.val());
    case OptionalState::false_value:
      return false_cast_to_array<T>();
    case OptionalState::null_value:
      return array<T>{};
    default:
      __builtin_unreachable();
  }
}

template<class T>
inline array<T> f$arrayval(Optional<T> &&val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(std::move(val.val()));
    case OptionalState::false_value:
      return false_cast_to_array<T>();
    case OptionalState::null_value:
      return array<T>{};
    default:
      __builtin_unreachable();
  }
}

template<class T>
inline array<T> f$arrayval(const Optional<array<T>> &val) {
  if (val.value_state() == OptionalState::false_value) {
    return false_cast_to_array<T>();
  }
  return val.val();
}

template<class T>
inline array<T> f$arrayval(Optional<array<T>> &&val) {
  if (val.value_state() == OptionalState::false_value) {
    return false_cast_to_array<T>();
  }
  return std::move(val.val());
}
