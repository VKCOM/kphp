#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<class T, class = enable_for_bool_int_double<T>>
inline bool f$boolval(T val) {
  return static_cast<bool>(val);
}

inline bool f$boolval(tmp_string val) {
  return val.to_bool();
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

//inline bool f$boolval(const mixed &val) {
//  return val.to_bool();
//}

template<class T, class = enable_for_bool_int_double<T>>
inline int64_t f$intval(T val) /*ubsan_supp("float-cast-overflow")*/;

template<class T, class>
inline int64_t f$intval(T val) {
  return static_cast<int64_t>(val);
}

inline int64_t f$intval(const string &val) {
  return val.to_int();
}

//inline int64_t f$intval(const mixed &val) {
//  return val.to_int();
//}

template<class T>
inline int64_t f$intval(const Optional<T> &val) {
  return val.has_value() ? f$intval(val.val()) : 0;
}

inline int64_t f$intval(tmp_string s) {
  if (!s.has_value()) {
    return 0;
  }
  return string::to_int(s.data, s.size);
}


inline int64_t f$safe_intval(bool val) {
  return val;
}

inline  int64_t f$safe_intval(int64_t val) {
  return val;
}

inline int64_t f$safe_intval(double val) {
  constexpr auto max_int = static_cast<double>(static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1);
  if (fabs(val) > max_int) {
    php_warning("Wrong conversion from double %.6lf to int", val);
  }
  return static_cast<int64_t>(val);
}

inline int64_t f$safe_intval(const string &val) {
  return val.safe_to_int();
}

inline int64_t f$safe_intval(const mixed &val) {
  return val.safe_to_int();
}

template<class T, class = enable_for_bool_int_double<T>>
inline double f$floatval(T val) {
  return static_cast<double>(val);
}

inline double f$floatval(const string &val) {
  return val.to_float();
}

inline double f$floatval(const mixed &val) {
  return val.to_float();
}

template<class T>
inline double f$floatval(const Optional<T> &val) {
  return val.has_value() ? f$floatval(val.val()) : 0;
}


inline string f$strval(bool val) {
  return (val ? string("1", 1) : string());
}

inline string f$strval(int64_t val) {
  return string(val);
}

inline string f$strval(double val) {
  return string(val);
}

inline string f$strval(const string &val) {
  return val;
}

inline string &&f$strval(string &&val) {
  return std::move(val);
}

template<class T>
inline string f$strval(const Optional<T> &val) {
  return val.has_value() ? f$strval(val.val()) : f$strval(false);
}

template<class T>
inline string f$strval(Optional<T> &&val) {
  return val.has_value() ? f$strval(std::move(val.val())) : f$strval(false);
}

inline string f$strval(const mixed &val) {
  return val.to_string();
}

inline string f$strval(mixed &&val) {
  return val.is_string() ? std::move(val.as_string()) : val.to_string();
}

template<class T>
inline array<T> f$arrayval(const T &val) {
  array<T> res(array_size(1, true));
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

//inline array<mixed> f$arrayval(const mixed &val) {
//  return val.to_array();
//}
//
//inline array<mixed> f$arrayval(mixed &&val) {
//  return val.is_array() ? std::move(val.as_array()) : val.to_array();
//}

namespace impl_ {

template<typename T>
inline std::enable_if_t<vk::is_type_in_list<T, bool, mixed>{}, array<T>> false_cast_to_array() {
  return f$arrayval(T{false});
}

template<typename T>
inline std::enable_if_t<!vk::is_type_in_list<T, bool, mixed>{}, array<T>> false_cast_to_array() {
  php_warning("Dangerous cast false to array, the result will be different from PHP");
  return array<T>{};
}

} // namespace impl_

template<class T>
inline array<T> f$arrayval(const Optional<T> &val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(val.val());
    case OptionalState::false_value:
      return impl_::false_cast_to_array<T>();
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
      return impl_::false_cast_to_array<T>();
    case OptionalState::null_value:
      return array<T>{};
    default:
      __builtin_unreachable();
  }
}

template<class T>
inline array<T> f$arrayval(const Optional<array<T>> &val) {
  if (val.is_false()) {
    return impl_::false_cast_to_array<T>();
  }
  return val.val();
}

template<class T>
inline array<T> f$arrayval(Optional<array<T>> &&val) {
  if (val.is_false()) {
    return impl_::false_cast_to_array<T>();
  }
  return std::move(val.val());
}
