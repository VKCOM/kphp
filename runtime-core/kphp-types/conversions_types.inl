#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

template<class T, class = enable_for_bool_int_double<T>>
inline bool f$boolval(T val) {
  return static_cast<bool>(val);
}

template<typename Allocator>
inline bool f$boolval(__runtime_core::tmp_string<Allocator> val) {
  return val.to_bool();
}

template<typename Allocator>
inline bool f$boolval(const __string<Allocator> &val) {
  return val.to_bool();
}

template<class T, typename Allocator>
inline bool f$boolval(const __array<T, Allocator> &val) {
  return !val.empty();
}

template<class T, typename Allocator>
inline bool f$boolval(const __class_instance<T, Allocator> &val) {
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

template<typename Allocator>
inline bool f$boolval(const __mixed<Allocator> &val) {
  return val.to_bool();
}

template<class T, class = enable_for_bool_int_double<T>>
inline int64_t f$intval(T val) ubsan_supp("float-cast-overflow");

template<class T, class>
inline int64_t f$intval(T val) {
  return static_cast<int64_t>(val);
}

template<typename Allocator>
inline int64_t f$intval(const __string<Allocator> &val) {
  return val.to_int();
}

template<typename Allocator>
inline int64_t f$intval(const __mixed<Allocator> &val) {
  return val.to_int();
}

template<class T>
inline int64_t f$intval(const Optional<T> &val) {
  return val.has_value() ? f$intval(val.val()) : 0;
}

template<typename Allocator>
inline int64_t f$intval(__runtime_core::tmp_string<Allocator> s) {
  if (!s.has_value()) {
    return 0;
  }
  return __string<Allocator>::to_int(s.data, s.size);
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

template<typename Allocator>
inline int64_t f$safe_intval(const __string<Allocator> &val) {
  return val.safe_to_int();
}

template<typename Allocator>
inline int64_t f$safe_intval(const __mixed<Allocator> &val) {
  return val.safe_to_int();
}

template<class T, class = enable_for_bool_int_double<T>>
inline double f$floatval(T val) {
  return static_cast<double>(val);
}

template<typename Allocator>
inline double f$floatval(const __string<Allocator> &val) {
  return val.to_float();
}

template<typename Allocator>
inline double f$floatval(const __mixed<Allocator> &val) {
  return val.to_float();
}

template<class T>
inline double f$floatval(const Optional<T> &val) {
  return val.has_value() ? f$floatval(val.val()) : 0;
}

namespace impl_ {

template<typename T, typename Allocator>
inline std::enable_if_t<vk::is_type_in_list<T, bool, __mixed<Allocator>>{}, __array<T, Allocator>> false_cast_to_array() {
  return f$arrayval(T{false});
}

template<typename T, typename Allocator>
inline std::enable_if_t<!vk::is_type_in_list<T, bool, __mixed<Allocator>>{}, __array<T, Allocator>> false_cast_to_array() {
  php_warning("Dangerous cast false to array, the result will be different from PHP");
  return __array<T, Allocator>{};
}

} // namespace impl_

namespace __runtime_core {
// todo:core
template<typename Allocator>
inline __string<Allocator> strval(bool val) {
  return (val ? __string<Allocator>("1", 1) : __string<Allocator>());
}

template<typename Allocator>
inline __string<Allocator> strval(int64_t val) {
  return __string<Allocator>(val);
}

template<typename Allocator>
inline __string<Allocator> strval(double val) {
  return __string<Allocator>(val);
}

template<class T, typename Allocator>
inline __string<Allocator> strval(const Optional<T> &val) {
  return val.has_value() ? f$strval(val.val()) : strval<Allocator>(false);
}

template<class T, typename Allocator>
inline __string<Allocator> strval(Optional<T> &&val) {
  return val.has_value() ? f$strval(std::move(val.val())) : strval<Allocator>(false);
}

template<class T, typename Allocator>
inline __array<T, Allocator> arrayval(const T &val) {
  __array<T, Allocator> res(array_size(1, true));
  res.push_back(val);
  return res;
}

template<class T, typename Allocator>
inline __array<T, Allocator> arrayval(const Optional<T> &val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(val.val());
    case OptionalState::false_value:
      return ::impl_::false_cast_to_array<T, Allocator>();
    case OptionalState::null_value:
      return __array<T, Allocator>{};
    default:
      __builtin_unreachable();
  }
}

template<class T, typename Allocator>
inline __array<T, Allocator> arrayval(Optional<T> &&val) {
  switch (val.value_state()) {
    case OptionalState::has_value:
      return f$arrayval(std::move(val.val()));
    case OptionalState::false_value:
      return ::impl_::false_cast_to_array<T, Allocator>();
    case OptionalState::null_value:
      return __array<T, Allocator>{};
    default:
      __builtin_unreachable();
  }
}
}

template<typename Allocator>
inline __string<Allocator> f$strval(const __string<Allocator> &val) {
  return val;
}

template<typename Allocator>
inline __string<Allocator> &&f$strval(__string<Allocator> &&val) {
  return std::move(val);
}

template<typename Allocator>
inline __string<Allocator> f$strval(const __mixed<Allocator> &val) {
  return val.to_string();
}

template<typename Allocator>
inline __string<Allocator> f$strval(__mixed<Allocator> &&val) {
  return val.is_string() ? std::move(val.as_string()) : val.to_string();
}


template<class T, typename Allocator>
inline const __array<T, Allocator> &f$arrayval(const __array<T, Allocator> &val) {
  return val;
}

template<class T, typename Allocator>
inline __array<T, Allocator> &&f$arrayval(__array<T, Allocator> &&val) {
  return std::move(val);
}

template<typename Allocator>
inline __array<__mixed<Allocator>, Allocator> f$arrayval(const __mixed<Allocator> &val) {
  return val.to_array();
}

template<typename Allocator>
inline __array<__mixed<Allocator>, Allocator> f$arrayval(__mixed<Allocator> &&val) {
  return val.is_array() ? std::move(val.as_array()) : val.to_array();
}

template<class T, typename Allocator>
inline __array<T, Allocator> f$arrayval(const Optional<__array<T, Allocator>> &val) {
  if (val.is_false()) {
    return impl_::false_cast_to_array<T, Allocator>();
  }
  return val.val();
}

template<class T, typename Allocator>
inline __array<T, Allocator> f$arrayval(Optional<__array<T, Allocator>> &&val) {
  if (val.is_false()) {
    return impl_::false_cast_to_array<T, Allocator>();
  }
  return std::move(val.val());
}