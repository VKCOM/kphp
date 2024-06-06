// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <unistd.h>
#include <utility>

#include "common/algorithms/find.h"
#include "common/sanitizer.h"
#include "common/type_traits/list_of_types.h"

#include "runtime-core/include.h"
#include "runtime-core/kphp-types/decl/shape.h"
#include "runtime-core/kphp-types/kphp-type-traits.h"
#include "runtime-core/class-instance/refcountable-php-classes.h"

// order of includes below matters, be careful

#define INCLUDED_FROM_KPHP_CORE

#include "runtime-core/kphp-types/decl/string_decl.inl"
#include "runtime-core/kphp-types/decl/array_decl.inl"
#include "runtime-core/class-instance/class-instance-decl.inl"
#include "runtime-core/kphp-types/decl/mixed_decl.inl"
#include "runtime-core/kphp-types/decl/string_buffer_decl.inl"

#include "runtime-core/runtime-core-context.h"

#include "runtime-core/kphp-types/definition/string.inl"
#include "runtime-core/kphp-types/definition/array.inl"
#include "runtime-core/class-instance/class-instance.inl"
#include "runtime-core/kphp-types/definition/mixed.inl"
#include "runtime-core/kphp-types/definition/string_buffer.inl"
#include "runtime-core/kphp-types/conversions_types.inl"
#include "runtime-core/kphp-types/comparison_operators.inl"

#undef INCLUDED_FROM_KPHP_CORE

#define FFI_CALL(call) ({ dl::CriticalSectionGuard critical_section___; (call); })
#define FFI_INVOKE_CALLBACK(call) ({ dl::NonCriticalSectionGuard non_critical_section___; (call); })

#define SAFE_SET_OP(a, op, b, b_type) ({b_type b_tmp___ = b; a op std::move(b_tmp___);})
#define SAFE_SET_FUNC_OP(a, func, b, b_type) ({b_type b_tmp___ = b; func (a, b_tmp___);})
#define SAFE_INDEX(a, b, b_type) a[({b_type b_tmp___ = b; b_tmp___;})]
#define SAFE_SET_VALUE(a, b, b_type, c, c_type) ({b_type b_tmp___ = b; c_type c_tmp___ = c; (a).set_value (b_tmp___, c_tmp___);})
#define SAFE_PUSH_BACK(a, b, b_type) ({b_type b_tmp___ = b; a.push_back (b_tmp___);})
#define SAFE_PUSH_BACK_RETURN(a, b, b_type) ({b_type b_tmp___ = b; a.push_back_return (b_tmp___);})
#define NOERR(a, a_type) ({php_disable_warnings++; a_type a_tmp___ = a; php_disable_warnings--; a_tmp___;})
#define NOERR_VOID(a) ({php_disable_warnings++; a; php_disable_warnings--;})

#define f$likely likely
#define f$unlikely unlikely


template<typename T, typename ...Args>
void hard_reset_var(T &var, Args &&... args) noexcept {
  new(&var) T(std::forward<Args>(args)...);
}

inline constexpr int64_t operator "" _i64(unsigned long long int v) noexcept {
  return static_cast<int64_t>(v);
}

inline double divide(int64_t lhs, int64_t rhs);

inline double divide(double lhs, int64_t rhs);

template<typename Allocator>
inline double divide(const __string<Allocator> &lhs, int64_t rhs);

template<typename Allocator>
inline double divide(const __mixed<Allocator> &lhs, int64_t rhs);


inline double divide(int64_t lhs, double rhs);

inline double divide(double lhs, double rhs) ubsan_supp("float-divide-by-zero");

template<typename Allocator>
inline double divide(const __string<Allocator> &lhs, double rhs);

template<typename Allocator>
inline double divide(const __mixed<Allocator> &lhs, double rhs);


template<typename Allocator>
inline double divide(int64_t lhs, const __string<Allocator> &rhs);

template<typename Allocator>
inline double divide(double lhs, const __string<Allocator> &rhs);


template<typename Allocator>
inline double divide(const __string<Allocator> &lhs, const __string<Allocator> &rhs);

template<typename Allocator>
inline double divide(const __mixed<Allocator> &lhs, const __string<Allocator> &rhs);

template<typename Allocator>
inline double divide(int64_t lhs, const __mixed<Allocator> &rhs);

template<typename Allocator>
inline double divide(double lhs, const __mixed<Allocator> &rhs);

template<typename Allocator>
inline double divide(const __string<Allocator> &lhs, const __mixed<Allocator> &rhs);

template<typename Allocator>
inline double divide(const __mixed<Allocator> &lhs, const __mixed<Allocator> &rhs);


inline double divide(bool lhs, bool rhs);

template<class T>
inline double divide(bool lhs, const T &rhs);

template<class T>
inline double divide(const T &lhs, bool rhs);

template<class T, typename Allocator>
inline double divide(bool lhs, const __array<T, Allocator> &rhs);

template<class T, typename Allocator>
inline double divide(const __array<T, Allocator> &lhs, bool rhs);

template<class T, typename Allocator>
inline double divide(bool lhs, const __class_instance<T, Allocator> &rhs);

template<class T, typename Allocator>
inline double divide(const __class_instance<T, Allocator> &lhs, bool rhs);


template<class T, class T1, typename Allocator>
inline double divide(const __array<T, Allocator> &lhs, const T1 &rhs);

template<class T, class T1, typename Allocator>
inline double divide(const T1 &lhs, const __array<T, Allocator> &rhs);


template<class T, class T1, typename Allocator>
inline double divide(const __class_instance<T, Allocator> &lhs, const T1 &rhs);

template<class T, class T1, typename Allocator>
inline double divide(const T1 &lhs, const __class_instance<T, Allocator> &rhs);


template<class T, typename Allocator>
inline double divide(const __array<T, Allocator> &lhs, const __array<T, Allocator> &rhs);

template<class T, typename Allocator>
inline double divide(const __class_instance<T, Allocator> &lhs, const __class_instance<T, Allocator> &rhs);

template<class T, class T1, typename Allocator>
inline double divide(const __array<T, Allocator> &lhs, const __array<T1, Allocator> &rhs);

template<class T, typename Allocator>
inline double divide(const __class_instance<T, Allocator> &lhs, const __class_instance<T, Allocator> &rhs);

template<class T, class T1, typename Allocator>
inline double divide(const __array<T, Allocator> &lhs, const __class_instance<T, Allocator> &rhs);

template<class T, typename Allocator>
inline double divide(const __class_instance<T, Allocator> &lhs, const __array<T, Allocator> &rhs);


template<class T1, class T2>
inline double divide(const Optional<T1> &lhs, const T2 &rhs); //not defined

template<class T1, class T2>
inline double divide(const T1 &lhs, const Optional<T2> &rhs); //not defined

template<class T1, class T2>
inline int64_t spaceship(const T1 &lhs, const T2 &rhs) {
  if (lt(lhs, rhs)) {
    return -1;
  }
  return eq2(lhs, rhs) ? 0 : 1;
}

inline int64_t modulo(int64_t lhs, int64_t rhs);

template<class T1, class T2>
inline int64_t modulo(const T1 &lhs, const T2 &rhs);

inline int64_t int_power(int64_t base, int64_t exp) {
  int64_t res = 1;
  while (exp > 0) {
    if (exp & 1) {
      res *= base;
    }
    base *= base;
    exp >>= 1;
  }
  return res;
}

inline double float_power(double base, int64_t exp) {
  return std::pow(base, exp);
}

template<typename Allocator>
inline __mixed<Allocator> var_power(const __mixed<Allocator> &base, const __mixed<Allocator> &exp) {
  if (base.is_int() && exp.is_int() && exp.to_int() >= 0) {
    return int_power(base.to_int(), exp.to_int());
  }
  const double float_base = base.to_float();
  if (exp.is_int()) {
    return std::pow(float_base, exp.to_int());
  }

  const double float_exp = exp.to_float();
  if (float_exp == static_cast<double>(static_cast<int64_t>(float_exp))) {
    return std::pow(float_base, float_exp);
  }

  if (base < 0.0) {
    php_warning("Calculating pow with negative base and double exp will produce zero");
    return 0.0;
  }

  return std::pow(float_base, float_exp);
}

inline int64_t &power_self(int64_t &base, int64_t exp) {
  return base = int_power(base, exp);
}

inline double &power_self(double &base, int64_t exp) {
  return base = float_power(base, exp);
}

template<typename Allocator>
inline __mixed<Allocator> &power_self(__mixed<Allocator> &base, const __mixed<Allocator> &exp) {
  return base = var_power(base, exp);
}

template<class T1, class T2>
inline T1 &divide_self(T1 &lhs, const T2 &rhs);


inline int64_t &modulo_self(int64_t &lhs, int64_t rhs);

template<class T1, class T2>
inline T1 &modulo_self(T1 &lhs, const T2 &rhs);


template<class T0, class T>
inline void assign(T0 &dest, const T &from);


inline bool &boolval_ref(bool &val);

template<typename Allocator>
inline bool &boolval_ref(__mixed<Allocator> &val);

inline const bool &boolval_ref(const bool &val);

template<typename Allocator>
inline const bool &boolval_ref(const __mixed<Allocator> &val);


inline int64_t &intval_ref(int64_t &val, const char *function);

template<typename Allocator>
inline int64_t &intval_ref(__mixed<Allocator> &val, const char *function);

inline const int64_t &intval_ref(const int64_t &val, const char *function);

template<typename Allocator>
inline const int64_t &intval_ref(const __mixed<Allocator> &val, const char *function);


inline double &floatval_ref(double &val);

template<typename Allocator>
inline double &floatval_ref(__mixed<Allocator> &val);

inline const double &floatval_ref(const double &val);

template<typename Allocator>
inline const double &floatval_ref(const __mixed<Allocator> &val);


template<typename Allocator>
inline __string<Allocator> &strval_ref(__string<Allocator> &val, const char *function);

template<typename Allocator>
inline __string<Allocator> &strval_ref(__mixed<Allocator> &val, const char *function);

template<typename Allocator>
inline const __string<Allocator> &strval_ref(const __string<Allocator> &val, const char *function);

template<typename Allocator>
inline const __string<Allocator> &strval_ref(const __mixed<Allocator> &val, const char *function);


template<class T, typename Allocator>
inline __array<T, Allocator> &arrayval_ref(__array<T, Allocator> &val, const char *function);

template<typename Allocator>
inline __array<__mixed<Allocator>, Allocator> &arrayval_ref(__mixed<Allocator> &val, const char *function);

template<class T, typename Allocator>
inline __array<T, Allocator> &arrayval_ref(Optional<__array<T, Allocator>> &val, const char *function);

template<class T, typename Allocator>
inline const __array<T, Allocator> &arrayval_ref(const __array<T, Allocator> &val, const char *function);

template<typename Allocator>
inline const __array<__mixed<Allocator>, Allocator> &arrayval_ref(const __mixed<Allocator> &val, const char *function);

template<class T, typename Allocator>
inline const __array<T, Allocator> &arrayval_ref(const Optional<__array<T, Allocator>> &val, const char *function);

template<class T, class = enable_for_bool_int_double<T>>
inline bool f$empty(const T &v);
template<typename Allocator>
inline bool f$empty(const __string<Allocator> &v);
template<typename Allocator>
inline bool f$empty(const __mixed<Allocator> &v);
template<class T, typename Allocator>
inline bool f$empty(const __array<T, Allocator> &);
template<class T, typename Allocator>
inline bool f$empty(const __class_instance<T, Allocator> &);
template<class T>
inline bool f$empty(const Optional<T> &);


template<class T>
bool f$is_numeric(const T &);
template<class T>
inline bool f$is_numeric(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_numeric(const __string<Allocator> &v);
template<typename Allocator>
inline bool f$is_numeric(const __mixed<Allocator> &v);

template<class T>
inline bool f$is_bool(const T &v);
template<class T>
inline bool f$is_bool(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_bool(const __mixed<Allocator> &v);

template<class T>
inline bool f$is_int(const T &);
template<class T>
inline bool f$is_int(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_int(const __mixed<Allocator> &v);

template<class T>
inline bool f$is_float(const T &v);
template<class T>
inline bool f$is_float(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_float(const __mixed<Allocator> &v);


template<class T>
inline bool f$is_scalar(const T &v);
template<class T>
inline bool f$is_scalar(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_scalar(const __mixed<Allocator> &v);


template<class T>
inline bool f$is_string(const T &v);
template<class T>
inline bool f$is_string(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_string(const __mixed<Allocator> &v);


template<class T>
inline bool f$is_array(const T &v);
template<class T>
inline bool f$is_array(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_array(const __mixed<Allocator> &v);


template<class T>
bool f$is_object(const T &);
template<class T, typename Allocator>
inline bool f$is_object(const __class_instance<T, Allocator> &v);


template<class T>
inline bool f$is_integer(const T &v);

template<class T>
inline bool f$is_long(const T &v);

template<class T>
inline bool f$is_double(const T &v);

template<class T>
inline bool f$is_real(const T &v);

template<class Derived, class Base, typename Allocator>
inline bool f$is_a(const __class_instance<Base, Allocator> &base) {
  return base.template is_a<Derived>();
}

template<class ClassInstanceDerived, class Base, typename Allocator>
inline ClassInstanceDerived f$instance_cast(const __class_instance<Base, Allocator> &base, const __string<Allocator> &) {
  return base.template cast_to<typename ClassInstanceDerived::ClassType>();
}

inline const char *get_type_c_str(bool);
inline const char *get_type_c_str(int64_t);
inline const char *get_type_c_str(double);
template<typename Allocator>
inline const char *get_type_c_str(const __string<Allocator> &v);
template<typename Allocator>
inline const char *get_type_c_str(const __mixed<Allocator> &v);
template<class T, typename Allocator>
inline const char *get_type_c_str(const __array<T, Allocator> &v);
template<class T, typename Allocator>
inline const char *get_type_c_str(const __class_instance<T, Allocator> &v);

template<typename Allocator>
inline __string<Allocator> f$get_class(const __string<Allocator> &v);
template<typename Allocator>
inline __string<Allocator> f$get_class(const __mixed<Allocator> &v);
template<class T, typename Allocator>
inline __string<Allocator> f$get_class(const __array<T, Allocator> &v);
template<class T, typename Allocator>
inline __string<Allocator> f$get_class(const __class_instance<T, Allocator> &v);

template<class T, typename Allocator>
inline int64_t f$get_hash_of_class(const __class_instance<T, Allocator> &klass);

template<typename Allocator>
inline int64_t f$count(const __mixed<Allocator> &v);

template<class T>
inline int64_t f$count(const Optional<T> &a);

template<class T, typename Allocator>
inline int64_t f$count(const __array<T, Allocator> &a);

template<class ...Args>
inline int64_t f$count(const std::tuple<Args...> &a);

template<class T>
inline int64_t f$count(const T &v);


template<class T>
int64_t f$sizeof(const T &v);

template<typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, __runtime_core::tmp_string<Allocator> from);
template<typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, const __string<Allocator> &from);
template<typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, int64_t from);

template<class T, typename Allocator>
inline __string<Allocator> &append(Optional<__string<Allocator>> &dest, const T &from);

template<class T, typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, const T &from);

template<class T, typename Allocator>
inline __mixed<Allocator> &append(__mixed<Allocator> &dest, const T &from);

template<class T0, class T>
inline T0 &append(T0 &dest, const T &from);

template<typename Allocator>
inline __string<Allocator> f$gettype(const __mixed<Allocator> &v);


template<class T>
inline bool f$function_exists(const T &a1);


constexpr int32_t E_ERROR = 1;
constexpr int32_t E_WARNING = 2;
constexpr int32_t E_PARSE = 4;
constexpr int32_t E_NOTICE = 8;
constexpr int32_t E_CORE_ERROR = 16;
constexpr int32_t E_CORE_WARNING = 32;
constexpr int32_t E_COMPILE_ERROR = 64;
constexpr int32_t E_COMPILE_WARNING = 128;
constexpr int32_t E_USER_ERROR = 256;
constexpr int32_t E_USER_WARNING = 512;
constexpr int32_t E_USER_NOTICE = 1024;
constexpr int32_t E_STRICT = 2048;
constexpr int32_t E_RECOVERABLE_ERROR = 4096;
constexpr int32_t E_DEPRECATED = 8192;
constexpr int32_t E_USER_DEPRECATED = 16384;
constexpr int32_t E_ALL = 32767;

template<typename Allocator>
inline __mixed<Allocator> f$error_get_last();

template<typename Allocator>
inline void f$warning(const __string<Allocator> &message);

#define f$critical_error(message) \
  php_critical_error("%s", message.c_str());

template<class T, typename Allocator>
inline int64_t f$get_reference_counter(const __array<T, Allocator> &v);

template<class T, typename Allocator>
inline int64_t f$get_reference_counter(const __class_instance<T, Allocator> &v);

template<typename Allocator>
inline int64_t f$get_reference_counter(const __string<Allocator> &v);

template<typename Allocator>
inline int64_t f$get_reference_counter(const __mixed<Allocator> &v);

template<typename Arg, typename Allocator = DefaultAllocator,
         typename = std::enable_if_t<std::is_constructible_v<__mixed<Allocator>, Arg> && (!std::is_same_v<Arg, __mixed<>>)>>
inline int64_t f$get_reference_counter(Arg &&m) {
  return f$get_reference_counter(__mixed<>{std::forward<Arg>(m)});
}


template<class T>
inline T &val(T &x);

template<class T>
inline const T &val(const T &x);

template<class T>
inline T &ref(T &x);

template<class T>
inline const T &val(const Optional<T> &x);

template<class T>
inline T &val(Optional<T> &x);

template<class T>
inline T &ref(Optional<T> &x);


template<class T, typename Allocator>
inline typename __array<T, Allocator>::iterator begin(__array<T, Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator begin(const __array<T, Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator const_begin(const __array<T, Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::iterator begin(__mixed<Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::const_iterator begin(const __mixed<Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::const_iterator const_begin(const __mixed<Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::iterator begin(Optional<__array<T, Allocator>> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator begin(const Optional<__array<T, Allocator>> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator const_begin(const Optional<__array<T, Allocator>> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::iterator end(__array<T, Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator end(const __array<T, Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator const_end(const __array<T, Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::iterator end(__mixed<Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::const_iterator end(const __mixed<Allocator> &x);

template<typename Allocator>
inline typename __array<__mixed<Allocator>, Allocator>::const_iterator const_end(const __mixed<Allocator> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::iterator end(Optional<__array<T, Allocator>> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator end(const Optional<__array<T, Allocator>> &x);

template<class T, typename Allocator>
inline typename __array<T, Allocator>::const_iterator const_end(const Optional<__array<T, Allocator>> &x);


template<typename Allocator>
inline void clear_array(__mixed<Allocator> &v);

template<class T, typename Allocator>
inline void clear_array(__array<T, Allocator> &a);

template<class T, typename Allocator>
inline void clear_array(Optional<__array<T, Allocator>> &a);

template<class T, typename Allocator>
inline void unset(__array<T, Allocator> &x);

template<class T, typename Allocator>
inline void unset(__class_instance<T, Allocator> &x);

template<typename Allocator>
inline void unset(__mixed<Allocator> &x);


/*
 *
 *     IMPLEMENTATION
 *
 */

double divide(int64_t lhs, int64_t rhs) {
  return divide(static_cast<double>(lhs), static_cast<double>(rhs));
}

double divide(double lhs, int64_t rhs) {
  return divide(lhs, static_cast<double>(rhs));
}

template<typename Allocator>
double divide(const __string<Allocator> &lhs, int64_t rhs) {
  return divide(f$floatval(lhs), rhs);
}

template<typename Allocator>
double divide(const __mixed<Allocator> &lhs, int64_t rhs) {
  return divide(f$floatval(lhs), rhs);
}


double divide(int64_t lhs, double rhs) {
  return divide(static_cast<double>(lhs), rhs);
}

double divide(double lhs, double rhs) {
  if (rhs == 0) {
    php_warning("Float division by zero");
  }

  return lhs / rhs;
}

template<typename Allocator>
double divide(const __string<Allocator> &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}

template<typename Allocator>
double divide(const __mixed<Allocator> &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}


template<typename Allocator>
double divide(int64_t lhs, const __string<Allocator> &rhs) {
  return divide(lhs, f$floatval(rhs));
}

template<typename Allocator>
double divide(double lhs, const __string<Allocator> &rhs) {
  return divide(lhs, f$floatval(rhs));
}

template<typename Allocator>
double divide(const __string<Allocator> &lhs, const __string<Allocator> &rhs) {
  return divide(f$floatval(lhs), f$floatval(rhs));
}

template<typename Allocator>
double divide(const __mixed<Allocator> &lhs, const __string<Allocator> &rhs) {
  return divide(lhs, f$floatval(rhs));
}


template<typename Allocator>
double divide(int64_t lhs, const __mixed<Allocator> &rhs) {
  return divide(lhs, f$floatval(rhs));
}

template<typename Allocator>
double divide(double lhs, const __mixed<Allocator> &rhs) {
  return divide(lhs, f$floatval(rhs));
}

template<typename Allocator>
double divide(const __string<Allocator> &lhs, const __mixed<Allocator> &rhs) {
  return divide(f$floatval(lhs), rhs);
}

template<typename Allocator>
double divide(const __mixed<Allocator> &lhs, const __mixed<Allocator> &rhs) {
  return divide(f$floatval(lhs), f$floatval(rhs));
}


double divide(bool lhs, bool rhs) {
  php_warning("Both arguments of operator '/' are bool");
  return divide(static_cast<int64_t>(lhs), static_cast<int64_t>(rhs));
}

template<class T>
double divide(bool lhs, const T &rhs) {
  php_warning("First argument of operator '/' is bool");
  return divide(static_cast<int64_t>(lhs), f$floatval(rhs));
}

template<class T>
double divide(const T &lhs, bool rhs) {
  php_warning("Second argument of operator '/' is bool");
  return divide(f$floatval(lhs), static_cast<int64_t>(rhs));
}

template<class T, typename Allocator>
double divide(bool, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' bool and array");
  return 0.0;
}

template<class T, typename Allocator>
double divide(const __array<T, Allocator> &, bool) {
  php_warning("Unsupported operand types for operator '/' array and bool");
  return 0.0;
}

template<class T, typename Allocator>
double divide(bool, const __class_instance<T, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' bool and object");
  return 0.0;
}

template<class T, typename Allocator>
double divide(const __class_instance<T, Allocator> &, bool) {
  php_warning("Unsupported operand types for operator '/' object and bool");
  return 0.0;
}


template<class T, class T1, typename Allocator>
double divide(const __array<T, Allocator> &lhs, const T1 &rhs) {
  php_warning("First argument of operator '/' is array");
  return divide(f$count(lhs), rhs);
}

template<class T, class T1, typename Allocator>
double divide(const T1 &lhs, const __array<T, Allocator> &rhs) {
  php_warning("Second argument of operator '/' is array");
  return divide(lhs, f$count(rhs));
}


template<class T, class T1, typename Allocator>
double divide(const __class_instance<T, Allocator> &, const T1 &rhs) {
  php_warning("First argument of operator '/' is object");
  return divide(1.0, rhs);
}

template<class T, class T1, typename Allocator>
double divide(const T1 &lhs, const __class_instance<T, Allocator> &) {
  php_warning("Second argument of operator '/' is object");
  return lhs;
}


template<class T, typename Allocator>
double divide(const __array<T, Allocator> &, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' array and array");
  return 0.0;
}

template<class T, typename Allocator>
double divide(const __class_instance<T, Allocator> &, const __class_instance<T, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' object and object");
  return 0.0;
}

template<class T, class T1, typename Allocator>
double divide(const __array<T, Allocator> &, const __array<T1, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' array and array");
  return 0.0;
}

template<class T, class T1, typename Allocator>
double divide(const __class_instance<T, Allocator> &, const __class_instance<T1, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' object and object");
  return 0.0;
}

template<class T, class T1, typename Allocator>
double divide(const __array<T, Allocator> &, const __class_instance<T1, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' array and object");
  return 0.0;
}

template<class T, class T1, typename Allocator>
double divide(const __class_instance<T1, Allocator> &, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator '/' object and array");
  return 0.0;
}


int64_t modulo(int64_t lhs, int64_t rhs) {
  if (rhs == 0) {
    php_warning("Modulo by zero");
    return 0;
  }
  return lhs % rhs;
}

template<class T1, class T2>
int64_t modulo(const T1 &lhs, const T2 &rhs) {
  int64_t div = f$intval(lhs);
  int64_t mod = f$intval(rhs);

  if (!eq2(div, lhs)) {
    php_warning("First parameter of operator %% is not an integer");
  }
  if (!eq2(mod, rhs)) {
    php_warning("Second parameter of operator %% is not an integer");
  }

  if (mod == 0) {
    php_warning("Modulo by zero");
    return 0;
  }
  return div % mod;
}


template<class T1, class T2>
T1 &divide_self(T1 &lhs, const T2 &rhs) {
  return lhs = divide(lhs, rhs);
}


int64_t &modulo_self(int64_t &lhs, int64_t rhs) {
  return lhs = modulo(lhs, rhs);
}

template<class T1, class T2>
T1 &modulo_self(T1 &lhs, const T2 &rhs) {
  return lhs = modulo(lhs, rhs);
}


template<class T0, class T>
void assign(T0 &dest, const T &from) {
  dest = from;
}

bool &boolval_ref(bool &val) {
  return val;
}

template<typename Allocator>
bool &boolval_ref(__mixed<Allocator> &val) {
  return val.as_bool("unknown");
}

const bool &boolval_ref(const bool &val) {
  return val;
}

template<typename Allocator>
const bool &boolval_ref(const __mixed<Allocator> &val) {
  return val.as_bool("unknown");
}


int64_t &intval_ref(int64_t &val, const char *) {
  return val;
}

template<typename Allocator>
int64_t &intval_ref(__mixed<Allocator> &val, const char *function) {
  return val.as_int(function);
}

const int64_t &intval_ref(const int64_t &val, const char *) {
  return val;
}

template<typename Allocator>
const int64_t &intval_ref(const __mixed<Allocator> &val, const char *function) {
  return val.as_int(function);
}


double &floatval_ref(double &val) {
  return val;
}

template<typename Allocator>
double &floatval_ref(__mixed<Allocator> &val) {
  return val.as_float("unknown");
}

const double &floatval_ref(const double &val) {
  return val;
}

template<typename Allocator>
const double &floatval_ref(const __mixed<Allocator> &val) {
  return val.as_float("unknown");
}


template<typename Allocator>
__string<Allocator> &strval_ref(__string<Allocator> &val, const char *) {
  return val;
}

template<typename Allocator>
__string<Allocator> &strval_ref(__mixed<Allocator> &val, const char *function) {
  return val.as_string(function);
}

template<typename Allocator>
const __string<Allocator> &strval_ref(const __string<Allocator> &val, const char *) {
  return val;
}

template<typename Allocator>
const __string<Allocator> &strval_ref(const __mixed<Allocator> &val, const char *function) {
  return val.as_string(function);
}


template<class T, typename Allocator>
__array<T, Allocator> &arrayval_ref(__array<T, Allocator> &val, const char *) {
  return val;
}

template<typename Allocator>
__array<__mixed<Allocator>, Allocator> &arrayval_ref(__mixed<Allocator> &val, const char *function) {
  return val.as_array(function);
}

template<class T, typename Allocator>
__array<T, Allocator> &arrayval_ref(Optional<__array<T, Allocator>> &val, const char *function) {
  if (unlikely(!val.has_value())) {
    php_warning("%s() expects parameter to be array, null or false is given", function);
  }
  return val.ref();
}

template<class T, typename Allocator>
const __array<T, Allocator> &arrayval_ref(const __array<T, Allocator> &val, const char *) {
  return val;
}

template<typename Allocator>
const __array<__mixed<Allocator>, Allocator> &arrayval_ref(const __mixed<Allocator> &val, const char *function) {
  return val.as_array(function);
}

template<class T, typename Allocator>
const __array<T, Allocator> &arrayval_ref(const Optional<__array<T, Allocator>> &val, const char *function) {
  if (unlikely(!val.has_value())) {
    php_warning("%s() expects parameter to be array, null or false is given", function);
  }
  return val.val();
}

template<class T>
const T &convert_to<T>::convert(const T &val) {
  return val;
}

template<class T>
T &&convert_to<T>::convert(T &&val) {
  return std::move(val);
}

template<class T>
T convert_to<T>::convert(const Unknown &) {
  return T();
}

template<class T>
template<class T1, class, class>
T convert_to<T>::convert(T1 &&val) {
  return T(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
bool convert_to<bool>::convert(T1 &&val) {
  return f$boolval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
int64_t convert_to<int64_t>::convert(T1 &&val) {
  return f$intval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
double convert_to<double>::convert(T1 &&val) {
  return f$floatval(std::forward<T1>(val));
}

template<class T, class>
inline bool f$empty(const T &v) {
  return v == 0;
}

template<typename Allocator>
bool f$empty(const __string<Allocator> &v) {
  typename __string<Allocator>::size_type l = v.size();
  return l == 0 || (l == 1 && v[0] == '0');
}

template<class T, typename Allocator>
bool f$empty(const __array<T, Allocator> &a) {
  return a.empty();
}

template<class T, typename Allocator>
bool f$empty(const __class_instance<T, Allocator> &o) {
  return o.is_null();   // false/null inside instance (in PHP, empty(false)=true behaves identically)
}

template<typename Allocator>
bool f$empty(const __mixed<Allocator> &v) {
  return v.empty();
}

template<class T>
bool f$empty(const Optional<T> &a) {
  return a.has_value() ? f$empty(a.val()) : true;
}

template<typename Allocator>
int64_t f$count(const __mixed<Allocator> &v) {
  return v.count();
}

template<class T>
int64_t f$count(const Optional<T> &a) {
  auto count_lambda = [](const auto &v) { return f$count(v);};
  return call_fun_on_optional_value(count_lambda, a);
}

template<class T, typename Allocator>
int64_t f$count(const __array<T, Allocator> &a) {
  return a.count();
}

template<class ...Args>
inline int64_t f$count(const std::tuple<Args...> &a __attribute__ ((unused))) {
  return static_cast<int64_t>(std::tuple_size<std::tuple<Args...>>::value);
}

template<class T>
int64_t f$count(const T &) {
  php_warning("Count on non-array");
  return 1;
}


template<class T>
int64_t f$sizeof(const T &v) {
  return f$count(v);
}


template<class T>
bool f$is_numeric(const T &) {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");
  return std::is_same<T, int64_t>{} || std::is_same<T, double>{};
}

template<typename Allocator>
bool f$is_numeric(const __string<Allocator> &v) {
  return v.is_numeric();
}

template<typename Allocator>
bool f$is_numeric(const __mixed<Allocator> &v) {
  return v.is_numeric();
}

template<class T>
inline bool f$is_numeric(const Optional<T> &v) {
  return v.has_value() ? f$is_numeric(v.val()) : false;
}


template<class T>
bool f$is_null(const T &) {
  return false;
}

template<class T, typename Allocator>
bool f$is_null(const __class_instance<T, Allocator> &v) {
  return v.is_null();
}

template<class T>
inline bool f$is_null(const Optional<T> &v) {
  return v.has_value() ? f$is_null(v.val()) : v.value_state() == OptionalState::null_value;
}

template<typename Allocator>
bool f$is_null(const __mixed<Allocator> &v) {
  return v.is_null();
}


template<class T>
bool f$is_bool(const T &) {
  return std::is_same<T, bool>::value;
}

template<class T>
inline bool f$is_bool(const Optional<T> &v) {
  return v.has_value() ? f$is_bool(v.val()) : v.is_false();
}

template<typename Allocator>
bool f$is_bool(const __mixed<Allocator> &v) {
  return v.is_bool();
}


template<class T>
bool f$is_int(const T &) {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");
  return std::is_same<T, int64_t>{};
}

template<class T>
inline bool f$is_int(const Optional<T> &v) {
  return v.has_value() ? f$is_int(v.val()) : false;
}

template<typename Allocator>
bool f$is_int(const __mixed<Allocator> &v) {
  return v.is_int();
}


template<class T>
bool f$is_float(const T &) {
  return std::is_same<T, double>::value;
}

template<class T>
inline bool f$is_float(const Optional<T> &v) {
  return v.has_value() ? f$is_float(v.val()) : false;
}

template<typename Allocator>
bool f$is_float(const __mixed<Allocator> &v) {
  return v.is_float();
}


template<class T>
bool f$is_scalar(const T &) {
  return std::is_arithmetic<T>::value || is_instance_of<T, __string>::value;
}

template<class T>
inline bool f$is_scalar(const Optional<T> &v) {
  auto is_scalar_lambda = [](const auto &v) { return f$is_scalar(v);};
  return call_fun_on_optional_value(is_scalar_lambda, v);
}

template<typename Allocator>
bool f$is_scalar(const __mixed<Allocator> &v) {
  return v.is_scalar();
}


template<class T>
bool f$is_string(const T &) {
  return is_instance_of<T, __string>::value;
}

template<class T>
inline bool f$is_string(const Optional<T> &v) {
  return v.has_value() ? f$is_string(v.val()) : false;
}

template<typename Allocator>
bool f$is_string(const __mixed<Allocator> &v) {
  return v.is_string();
}


template<class T>
inline bool f$is_array(const T &) {
  return is_array<T>::value;
}

template<typename Allocator>
bool f$is_array(const __mixed<Allocator> &v) {
  return v.is_array();
}

template<class T>
inline bool f$is_array(const Optional<T> &v) {
  return v.has_value() ? f$is_array(v.val()) : false;
}

template<class T>
bool f$is_object(const T &) {
  return false;
}

template<class T, typename Allocator>
bool f$is_object(const __class_instance<T, Allocator> &v) {
  return !v.is_null();
}


template<class T>
bool f$is_integer(const T &v) {
  return f$is_int(v);
}

template<class T>
bool f$is_long(const T &v) {
  return f$is_int(v);
}

template<class T>
bool f$is_double(const T &v) {
  return f$is_float(v);
}

template<class T>
bool f$is_real(const T &v) {
  return f$is_float(v);
}

const char *get_type_c_str(bool) {
  return "boolean";
}

const char *get_type_c_str(int64_t) {
  return "integer";
}

const char *get_type_c_str(double) {
  return "double";
}

template<typename Allocator>
const char *get_type_c_str(const __string<Allocator> &) {
  return "__string<Allocator>";
}

template<typename Allocator>
const char *get_type_c_str(const __mixed<Allocator> &v) {
  return v.get_type_c_str();
}

template<class T, typename Allocator>
const char *get_type_c_str(const __array<T, Allocator> &) {
  return "array";
}

template<class T, typename Allocator>
const char *get_type_c_str(const __class_instance<T, Allocator> &) {
  return "object";
}

namespace __runtime_core {
template<typename Allocator, class T>
__string<Allocator> get_type(const T &v) {
  const char *res = get_type_c_str(v);
  return {res, static_cast<typename __string<Allocator>::size_type>(strlen(res))};
}

template<typename Allocator>
__string<Allocator> get_class(bool) {
  php_warning("Called get_class() on boolean");
  return {};
}

template<typename Allocator>
__string<Allocator> get_class(int64_t) {
  php_warning("Called get_class() on integer");
  return {};
}

template<typename Allocator>
__string<Allocator> get_class(double) {
  php_warning("Called get_class() on double");
  return {};
}
}

template<typename Allocator>
__string<Allocator> f$get_class(const __string<Allocator> &) {
  php_warning("Called get_class() on __string<Allocator>");
  return {};
}

template<typename Allocator>
__string<Allocator> f$get_class(const __mixed<Allocator> &v) {
  php_warning("Called get_class() on %s", v.get_type_c_str());
  return {};
}

template<class T, typename Allocator>
__string<Allocator> f$get_class(const __array<T, Allocator> &) {
  php_warning("Called get_class() on array");
  return {};
}

template<class T, typename Allocator>
__string<Allocator> f$get_class(const __class_instance<T, Allocator> &v) {
  return __string<Allocator>(v.get_class());
}

template<class T, typename Allocator>
inline int64_t f$get_hash_of_class(const __class_instance<T, Allocator> &klass) {
  return klass.get_hash();
}

template<typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, __runtime_core::tmp_string<Allocator> from) {
  return dest.append(from.data, from.size);
}

template<typename Allocator>
__string<Allocator> &append(__string<Allocator> &dest, const __string<Allocator> &from) {
  return dest.append(from);
}

template<typename Allocator>
inline __string<Allocator> &append(__string<Allocator> &dest, int64_t from) {
  return dest.append(from);
}

template<class T, typename Allocator>
__string<Allocator> &append(Optional<__string<Allocator>> &dest, const T &from) {
  return append(dest.ref(), from);
}

template<class T, typename Allocator>
__string<Allocator> &append(__string<Allocator> &dest, const T &from) {
  return dest.append(f$strval(from));
}

template<class T, typename Allocator>
__mixed<Allocator> &append(__mixed<Allocator> &dest, const T &from) {
  return dest.append(f$strval(from));
}
template<typename Allocator>
inline __mixed<Allocator> &append(__mixed<Allocator> &dest, __runtime_core::tmp_string<Allocator> from) {
  return dest.append(from);
}

template<class T0, class T>
T0 &append(T0 &dest, const T &from) {
  php_warning("Wrong arguments types %s and %s for operator .=", get_type_c_str(dest), get_type_c_str(from));
  return dest;
}

template<typename Allocator>
__string<Allocator> f$gettype(const __mixed<Allocator> &v) {
  return v.get_type_str();
}

template<typename Arg, typename Allocator = DefaultAllocator,
         typename = std::enable_if_t<std::is_constructible_v<__mixed<Allocator>, Arg> && (!std::is_same_v<Arg, __mixed<>>)>>
inline __string<Allocator> f$gettype(Arg &&m) {
  return f$gettype(__mixed<>{std::forward<Arg>(m)});
}

template<class T>
bool f$function_exists(const T &) {
  return true;
}

template<typename Allocator>
__mixed<Allocator> f$error_get_last() {
  return {};
}

template<typename Allocator>
void f$warning(const __string<Allocator> &message) {
  php_warning("%s", message.c_str());
}

template<class T, typename Allocator>
int64_t f$get_reference_counter(const __array<T, Allocator> &v) {
  return v.get_reference_counter();
}

template<class T, typename Allocator>
int64_t f$get_reference_counter(const __class_instance<T, Allocator> &v) {
  return v.get_reference_counter();
}

template<typename Allocator>
int64_t f$get_reference_counter(const __string<Allocator> &v) {
  return v.get_reference_counter();
}

template<typename Allocator>
int64_t f$get_reference_counter(const __mixed<Allocator> &v) {
  return v.get_reference_counter();
}


template<class T>
T &val(T &x) {
  return x;
}

template<class T>
const T &val(const T &x) {
  return x;
}

template<class T>
T &ref(T &x) {
  return x;
}

template<class T>
const T &val(const Optional<T> &x) {
  return x.val();
}

template<class T>
T &val(Optional<T> &x) {
  return x.val();
}

template<class T>
T &ref(Optional<T> &x) {
  return x.ref();
}


template<class T, typename Allocator>
typename __array<T, Allocator>::iterator begin(__array<T, Allocator> &x) {
  return x.begin();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator begin(const __array<T, Allocator> &x) {
  return x.begin();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator const_begin(const __array<T, Allocator> &x) {
  return x.begin();
}


template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::iterator begin(__mixed<Allocator> &x) {
  return x.begin();
}

template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::const_iterator begin(const __mixed<Allocator> &x) {
  return x.begin();
}

template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::const_iterator const_begin(const __mixed<Allocator> &x) {
  return x.begin();
}


template<class T, typename Allocator>
typename __array<T, Allocator>::iterator begin(Optional<__array<T, Allocator>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator begin(const Optional<__array<T, Allocator>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator const_begin(const Optional<__array<T, Allocator>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}


template<class T, typename Allocator>
typename __array<T, Allocator>::iterator end(__array<T, Allocator> &x) {
  return x.end();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator end(const __array<T, Allocator> &x) {
  return x.end();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator const_end(const __array<T, Allocator> &x) {
  return x.end();
}

template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::iterator end(__mixed<Allocator> &x) {
  return x.end();
}

template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::const_iterator end(const __mixed<Allocator> &x) {
  return x.end();
}

template<typename Allocator>
typename __array<__mixed<Allocator>, Allocator>::const_iterator const_end(const __mixed<Allocator> &x) {
  return x.end();
}


template<class T, typename Allocator>
typename __array<T, Allocator>::iterator end(Optional<__array<T, Allocator>> &x) {
  return x.val().end();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator end(const Optional<__array<T, Allocator>> &x) {
  return x.val().end();
}

template<class T, typename Allocator>
typename __array<T, Allocator>::const_iterator const_end(const Optional<__array<T, Allocator>> &x) {
  return x.val().end();
}


template<typename Allocator>
void clear_array(__mixed<Allocator> &v) {
  v.clear();
}

template<class T, typename Allocator>
void clear_array(__array<T, Allocator> &a) {
  a.clear();
}

template<class T, typename Allocator>
void clear_array(Optional<__array<T, Allocator>> &a) {
  a = false;
}

template<class T, typename Allocator>
void unset(__array<T, Allocator> &x) {
  x = {};
}

template<class T, typename Allocator>
void unset(__class_instance<T, Allocator> &x) {
  x = {};
}

template<typename Allocator>
void unset(__mixed<Allocator> &x) {
  x = {};
}

template<typename T>
inline decltype(auto) check_not_null(T&& val) {
  if (f$is_null(val)) {
    php_warning("not_null is called on null");
  }
  return std::forward<T>(val);
}

template<typename T>
inline decltype(auto) check_not_false(T&& val) {
  if (f$is_bool(val) && !f$boolval(val)) {
    php_warning("not_false is called on false");
  }
  return std::forward<T>(val);
}

template<typename T>
using future = int64_t;
template<typename T>
using future_queue = int64_t;
