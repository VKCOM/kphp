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

#include "kphp-core/include.h"
#include "kphp-core/kphp-types/kphp_type_traits.h"
#include "kphp-core/kphp-types/decl/shape.h"

// order of includes below matters, be careful

#define INCLUDED_FROM_KPHP_CORE

#include "kphp-core/kphp-types/decl/string_decl.inl"
#include "kphp-core/kphp-types/decl/array_decl.inl"
#include "kphp-core/class-instance/class_instance_decl.inl"
#include "kphp-core/kphp-types/decl/mixed_decl.inl"
#include "kphp-core/kphp-types/decl/string_buffer_decl.inl"

#include "kphp-core/kphp-core-context.h"

#include "kphp-core/kphp-types/definition/string.inl"
#include "kphp-core/kphp-types/definition/array.inl"
#include "kphp-core/class-instance/class_instance.inl"
#include "kphp-core/kphp-types/definition/mixed.inl"
#include "kphp-core/kphp-types/definition/string_buffer.inl"
#include "kphp-core/kphp-types/conversions_types.inl"
#include "kphp-core/kphp-types/comparison_operators.inl"

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

inline double divide(const string &lhs, int64_t rhs);

inline double divide(const mixed &lhs, int64_t rhs);


inline double divide(int64_t lhs, double rhs);

inline double divide(double lhs, double rhs) ubsan_supp("float-divide-by-zero");

inline double divide(const string &lhs, double rhs);

inline double divide(const mixed &lhs, double rhs);


inline double divide(int64_t lhs, const string &rhs);

inline double divide(double lhs, const string &rhs);

inline double divide(const string &lhs, const string &rhs);

inline double divide(const mixed &lhs, const string &rhs);


inline double divide(int64_t lhs, const mixed &rhs);

inline double divide(double lhs, const mixed &rhs);

inline double divide(const string &lhs, const mixed &rhs);

inline double divide(const mixed &lhs, const mixed &rhs);


inline double divide(bool lhs, bool rhs);

template<class T>
inline double divide(bool lhs, const T &rhs);

template<class T>
inline double divide(const T &lhs, bool rhs);

template<class T>
inline double divide(bool lhs, const array<T> &rhs);

template<class T>
inline double divide(const array<T> &lhs, bool rhs);

template<class T>
inline double divide(bool lhs, const class_instance<T> &rhs);

template<class T>
inline double divide(const class_instance<T> &lhs, bool rhs);


template<class T, class T1>
inline double divide(const array<T> &lhs, const T1 &rhs);

template<class T, class T1>
inline double divide(const T1 &lhs, const array<T> &rhs);


template<class T, class T1>
inline double divide(const class_instance<T> &lhs, const T1 &rhs);

template<class T, class T1>
inline double divide(const T1 &lhs, const class_instance<T> &rhs);


template<class T>
inline double divide(const array<T> &lhs, const array<T> &rhs);

template<class T>
inline double divide(const class_instance<T> &lhs, const class_instance<T> &rhs);

template<class T, class T1>
inline double divide(const array<T> &lhs, const array<T1> &rhs);

template<class T, class T1>
inline double divide(const class_instance<T> &lhs, const class_instance<T1> &rhs);

template<class T, class T1>
inline double divide(const array<T> &lhs, const class_instance<T1> &rhs);

template<class T, class T1>
inline double divide(const class_instance<T1> &lhs, const array<T> &rhs);


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

inline mixed var_power(const mixed &base, const mixed &exp) {
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

inline mixed &power_self(mixed &base, const mixed &exp) {
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

inline bool &boolval_ref(mixed &val);

inline const bool &boolval_ref(const bool &val);

inline const bool &boolval_ref(const mixed &val);


inline int64_t &intval_ref(int64_t &val, const char *function);

inline int64_t &intval_ref(mixed &val, const char *function);

inline const int64_t &intval_ref(const int64_t &val, const char *function);

inline const int64_t &intval_ref(const mixed &val, const char *function);


inline double &floatval_ref(double &val);

inline double &floatval_ref(mixed &val);

inline const double &floatval_ref(const double &val);

inline const double &floatval_ref(const mixed &val);


inline string &strval_ref(string &val, const char *function);

inline string &strval_ref(mixed &val, const char *function);

inline const string &strval_ref(const string &val, const char *function);

inline const string &strval_ref(const mixed &val, const char *function);


template<class T>
inline array<T> &arrayval_ref(array<T> &val, const char *function);

inline array<mixed> &arrayval_ref(mixed &val, const char *function);

template<class T>
inline array<T> &arrayval_ref(Optional<array<T>> &val, const char *function);

template<class T>
inline const array<T> &arrayval_ref(const array<T> &val, const char *function);

inline const array<mixed> &arrayval_ref(const mixed &val, const char *function);

template<class T>
inline const array<T> &arrayval_ref(const Optional<array<T>> &val, const char *function);

template<class T, class = enable_for_bool_int_double<T>>
inline bool f$empty(const T &v);
inline bool f$empty(const string &v);
inline bool f$empty(const mixed &v);
template<class T>
inline bool f$empty(const array<T> &);
template<class T>
inline bool f$empty(const class_instance<T> &);
template<class T>
inline bool f$empty(const Optional<T> &);


template<class T>
bool f$is_numeric(const T &);
template<class T>
inline bool f$is_numeric(const Optional<T> &v);
inline bool f$is_numeric(const string &v);
inline bool f$is_numeric(const mixed &v);

template<class T>
inline bool f$is_bool(const T &v);
template<class T>
inline bool f$is_bool(const Optional<T> &v);
inline bool f$is_bool(const mixed &v);

template<class T>
inline bool f$is_int(const T &);
template<class T>
inline bool f$is_int(const Optional<T> &v);
inline bool f$is_int(const mixed &v);

template<class T>
inline bool f$is_float(const T &v);
template<class T>
inline bool f$is_float(const Optional<T> &v);
inline bool f$is_float(const mixed &v);


template<class T>
inline bool f$is_scalar(const T &v);
template<class T>
inline bool f$is_scalar(const Optional<T> &v);
inline bool f$is_scalar(const mixed &v);


template<class T>
inline bool f$is_string(const T &v);
template<class T>
inline bool f$is_string(const Optional<T> &v);
inline bool f$is_string(const mixed &v);


template<class T>
inline bool f$is_array(const T &v);
template<class T>
inline bool f$is_array(const Optional<T> &v);
inline bool f$is_array(const mixed &v);


template<class T>
bool f$is_object(const T &);
template<class T>
inline bool f$is_object(const class_instance<T> &v);


template<class T>
inline bool f$is_integer(const T &v);

template<class T>
inline bool f$is_long(const T &v);

template<class T>
inline bool f$is_double(const T &v);

template<class T>
inline bool f$is_real(const T &v);

template<class Derived, class Base>
inline bool f$is_a(const class_instance<Base> &base) {
  return base.template is_a<Derived>();
}

template<class ClassInstanceDerived, class Base>
inline ClassInstanceDerived f$instance_cast(const class_instance<Base> &base, const string &) {
  return base.template cast_to<typename ClassInstanceDerived::ClassType>();
}

inline const char *get_type_c_str(bool);
inline const char *get_type_c_str(int64_t);
inline const char *get_type_c_str(double);
inline const char *get_type_c_str(const string &v);
inline const char *get_type_c_str(const mixed &v);
template<class T>
inline const char *get_type_c_str(const array<T> &v);
template<class T>
inline const char *get_type_c_str(const class_instance<T> &v);

template<class T>
inline string f$get_type(const T &v);

inline string f$get_class(bool);
inline string f$get_class(int64_t);
inline string f$get_class(double);
inline string f$get_class(const string &v);
inline string f$get_class(const mixed &v);
template<class T>
inline string f$get_class(const array<T> &v);
template<class T>
inline string f$get_class(const class_instance<T> &v);

template<class T>
inline int64_t f$get_hash_of_class(const class_instance<T> &klass);


inline int64_t f$count(const mixed &v);

template<class T>
inline int64_t f$count(const Optional<T> &a);

template<class T>
inline int64_t f$count(const array<T> &a);

template<class ...Args>
inline int64_t f$count(const std::tuple<Args...> &a);

template<class T>
inline int64_t f$count(const T &v);


template<class T>
int64_t f$sizeof(const T &v);


inline string &append(string &dest, tmp_string from);
inline string &append(string &dest, const string &from);
inline string &append(string &dest, int64_t from);

template<class T>
inline string &append(Optional<string> &dest, const T &from);

template<class T>
inline string &append(string &dest, const T &from);

template<class T>
inline mixed &append(mixed &dest, const T &from);

template<class T0, class T>
inline T0 &append(T0 &dest, const T &from);


inline string f$gettype(const mixed &v);

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

inline mixed f$error_get_last();

inline void f$warning(const string &message);

#define f$critical_error(message) \
  php_critical_error("%s", message.c_str());

template<class T>
inline int64_t f$get_reference_counter(const array<T> &v);

template<class T>
inline int64_t f$get_reference_counter(const class_instance<T> &v);

inline int64_t f$get_reference_counter(const string &v);

inline int64_t f$get_reference_counter(const mixed &v);


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


template<class T>
inline typename array<T>::iterator begin(array<T> &x);

template<class T>
inline typename array<T>::const_iterator begin(const array<T> &x);

template<class T>
inline typename array<T>::const_iterator const_begin(const array<T> &x);

inline array<mixed>::iterator begin(mixed &x);

inline array<mixed>::const_iterator begin(const mixed &x);

inline array<mixed>::const_iterator const_begin(const mixed &x);

template<class T>
inline typename array<T>::iterator begin(Optional<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator begin(const Optional<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator const_begin(const Optional<array<T>> &x);

template<class T>
inline typename array<T>::iterator end(array<T> &x);

template<class T>
inline typename array<T>::const_iterator end(const array<T> &x);

template<class T>
inline typename array<T>::const_iterator const_end(const array<T> &x);

inline array<mixed>::iterator end(mixed &x);

inline array<mixed>::const_iterator end(const mixed &x);

inline array<mixed>::const_iterator const_end(const mixed &x);

template<class T>
inline typename array<T>::iterator end(Optional<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator end(const Optional<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator const_end(const Optional<array<T>> &x);


inline void clear_array(mixed &v);

template<class T>
inline void clear_array(array<T> &a);

template<class T>
inline void clear_array(Optional<array<T>> &a);

template<class T>
inline void unset(array<T> &x);

template<class T>
inline void unset(class_instance<T> &x);

inline void unset(mixed &x);


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

double divide(const string &lhs, int64_t rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const mixed &lhs, int64_t rhs) {
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

double divide(const string &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const mixed &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}


double divide(int64_t lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(double lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(const string &lhs, const string &rhs) {
  return divide(f$floatval(lhs), f$floatval(rhs));
}

double divide(const mixed &lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}


double divide(int64_t lhs, const mixed &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(double lhs, const mixed &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(const string &lhs, const mixed &rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const mixed &lhs, const mixed &rhs) {
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

template<class T>
double divide(bool, const array<T> &) {
  php_warning("Unsupported operand types for operator '/' bool and array");
  return 0.0;
}

template<class T>
double divide(const array<T> &, bool) {
  php_warning("Unsupported operand types for operator '/' array and bool");
  return 0.0;
}

template<class T>
double divide(bool, const class_instance<T> &) {
  php_warning("Unsupported operand types for operator '/' bool and object");
  return 0.0;
}

template<class T>
double divide(const class_instance<T> &, bool) {
  php_warning("Unsupported operand types for operator '/' object and bool");
  return 0.0;
}


template<class T, class T1>
double divide(const array<T> &lhs, const T1 &rhs) {
  php_warning("First argument of operator '/' is array");
  return divide(f$count(lhs), rhs);
}

template<class T, class T1>
double divide(const T1 &lhs, const array<T> &rhs) {
  php_warning("Second argument of operator '/' is array");
  return divide(lhs, f$count(rhs));
}


template<class T, class T1>
double divide(const class_instance<T> &, const T1 &rhs) {
  php_warning("First argument of operator '/' is object");
  return divide(1.0, rhs);
}

template<class T, class T1>
double divide(const T1 &lhs, const class_instance<T> &) {
  php_warning("Second argument of operator '/' is object");
  return lhs;
}


template<class T>
double divide(const array<T> &, const array<T> &) {
  php_warning("Unsupported operand types for operator '/' array and array");
  return 0.0;
}

template<class T>
double divide(const class_instance<T> &, const class_instance<T> &) {
  php_warning("Unsupported operand types for operator '/' object and object");
  return 0.0;
}

template<class T, class T1>
double divide(const array<T> &, const array<T1> &) {
  php_warning("Unsupported operand types for operator '/' array and array");
  return 0.0;
}

template<class T, class T1>
double divide(const class_instance<T> &, const class_instance<T1> &) {
  php_warning("Unsupported operand types for operator '/' object and object");
  return 0.0;
}

template<class T, class T1>
double divide(const array<T> &, const class_instance<T1> &) {
  php_warning("Unsupported operand types for operator '/' array and object");
  return 0.0;
}

template<class T, class T1>
double divide(const class_instance<T1> &, const array<T> &) {
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

bool &boolval_ref(mixed &val) {
  return val.as_bool("unknown");
}

const bool &boolval_ref(const bool &val) {
  return val;
}

const bool &boolval_ref(const mixed &val) {
  return val.as_bool("unknown");
}


int64_t &intval_ref(int64_t &val, const char *) {
  return val;
}

int64_t &intval_ref(mixed &val, const char *function) {
  return val.as_int(function);
}

const int64_t &intval_ref(const int64_t &val, const char *) {
  return val;
}

const int64_t &intval_ref(const mixed &val, const char *function) {
  return val.as_int(function);
}


double &floatval_ref(double &val) {
  return val;
}

double &floatval_ref(mixed &val) {
  return val.as_float("unknown");
}

const double &floatval_ref(const double &val) {
  return val;
}

const double &floatval_ref(const mixed &val) {
  return val.as_float("unknown");
}


string &strval_ref(string &val, const char *) {
  return val;
}

string &strval_ref(mixed &val, const char *function) {
  return val.as_string(function);
}

const string &strval_ref(const string &val, const char *) {
  return val;
}

const string &strval_ref(const mixed &val, const char *function) {
  return val.as_string(function);
}


template<class T>
array<T> &arrayval_ref(array<T> &val, const char *) {
  return val;
}

array<mixed> &arrayval_ref(mixed &val, const char *function) {
  return val.as_array(function);
}

template<class T>
array<T> &arrayval_ref(Optional<array<T>> &val, const char *function) {
  if (unlikely(!val.has_value())) {
    php_warning("%s() expects parameter to be array, null or false is given", function);
  }
  return val.ref();
}

template<class T>
const array<T> &arrayval_ref(const array<T> &val, const char *) {
  return val;
}

const array<mixed> &arrayval_ref(const mixed &val, const char *function) {
  return val.as_array(function);
}

template<class T>
const array<T> &arrayval_ref(const Optional<array<T>> &val, const char *function) {
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

template<>
template<class T1, class, class>
string convert_to<string>::convert(T1 &&val) {
  return f$strval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
array<mixed> convert_to<array<mixed>>::convert(T1 &&val) {
  return f$arrayval(std::forward<T1>(val));
}

template<>
template<class T1, class, class>
mixed convert_to<mixed>::convert(T1 &&val) {
  return mixed{std::forward<T1>(val)};
}


template<class T, class>
inline bool f$empty(const T &v) {
  return v == 0;
}

bool f$empty(const string &v) {
  string::size_type l = v.size();
  return l == 0 || (l == 1 && v[0] == '0');
}

template<class T>
bool f$empty(const array<T> &a) {
  return a.empty();
}

template<class T>
bool f$empty(const class_instance<T> &o) {
  return o.is_null();   // false/null inside instance (in PHP, empty(false)=true behaves identically)
}

bool f$empty(const mixed &v) {
  return v.empty();
}

template<class T>
bool f$empty(const Optional<T> &a) {
  return a.has_value() ? f$empty(a.val()) : true;
}

int64_t f$count(const mixed &v) {
  return v.count();
}

template<class T>
int64_t f$count(const Optional<T> &a) {
  auto count_lambda = [](const auto &v) { return f$count(v);};
  return call_fun_on_optional_value(count_lambda, a);
}

template<class T>
int64_t f$count(const array<T> &a) {
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

bool f$is_numeric(const string &v) {
  return v.is_numeric();
}

bool f$is_numeric(const mixed &v) {
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

template<class T>
bool f$is_null(const class_instance<T> &v) {
  return v.is_null();
}

template<class T>
inline bool f$is_null(const Optional<T> &v) {
  return v.has_value() ? f$is_null(v.val()) : v.value_state() == OptionalState::null_value;
}

bool f$is_null(const mixed &v) {
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

bool f$is_bool(const mixed &v) {
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

bool f$is_int(const mixed &v) {
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

bool f$is_float(const mixed &v) {
  return v.is_float();
}


template<class T>
bool f$is_scalar(const T &) {
  return std::is_arithmetic<T>::value || std::is_same<T, string>::value;
}

template<class T>
inline bool f$is_scalar(const Optional<T> &v) {
  auto is_scalar_lambda = [](const auto &v) { return f$is_scalar(v);};
  return call_fun_on_optional_value(is_scalar_lambda, v);
}

bool f$is_scalar(const mixed &v) {
  return v.is_scalar();
}


template<class T>
bool f$is_string(const T &) {
  return std::is_same<T, string>::value;
}

template<class T>
inline bool f$is_string(const Optional<T> &v) {
  return v.has_value() ? f$is_string(v.val()) : false;
}

bool f$is_string(const mixed &v) {
  return v.is_string();
}


template<class T>
inline bool f$is_array(const T &) {
  return is_array<T>::value;
}

bool f$is_array(const mixed &v) {
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

template<class T>
bool f$is_object(const class_instance<T> &v) {
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

const char *get_type_c_str(const string &) {
  return "string";
}

const char *get_type_c_str(const mixed &v) {
  return v.get_type_c_str();
}

template<class T>
const char *get_type_c_str(const array<T> &) {
  return "array";
}

template<class T>
const char *get_type_c_str(const class_instance<T> &) {
  return "object";
}


template<class T>
string f$get_type(const T &v) {
  const char *res = get_type_c_str(v);
  return {res, static_cast<string::size_type>(strlen(res))};
}


string f$get_class(bool) {
  php_warning("Called get_class() on boolean");
  return {};
}

string f$get_class(int64_t) {
  php_warning("Called get_class() on integer");
  return {};
}

string f$get_class(double) {
  php_warning("Called get_class() on double");
  return {};
}

string f$get_class(const string &) {
  php_warning("Called get_class() on string");
  return {};
}

string f$get_class(const mixed &v) {
  php_warning("Called get_class() on %s", v.get_type_c_str());
  return {};
}

template<class T>
string f$get_class(const array<T> &) {
  php_warning("Called get_class() on array");
  return {};
}

template<class T>
string f$get_class(const class_instance<T> &v) {
  return string(v.get_class());
}

template<class T>
inline int64_t f$get_hash_of_class(const class_instance<T> &klass) {
  return klass.get_hash();
}

inline string &append(string &dest, tmp_string from) {
  return dest.append(from.data, from.size);
}

string &append(string &dest, const string &from) {
  return dest.append(from);
}

inline string &append(string &dest, int64_t from) {
  return dest.append(from);
}

template<class T>
string &append(Optional<string> &dest, const T &from) {
  return append(dest.ref(), from);
}

template<class T>
string &append(string &dest, const T &from) {
  return dest.append(f$strval(from));
}

template<class T>
mixed &append(mixed &dest, const T &from) {
  return dest.append(f$strval(from));
}

inline mixed &append(mixed &dest, tmp_string from) {
  return dest.append(from);
}

template<class T0, class T>
T0 &append(T0 &dest, const T &from) {
  php_warning("Wrong arguments types %s and %s for operator .=", get_type_c_str(dest), get_type_c_str(from));
  return dest;
}


string f$gettype(const mixed &v) {
  return v.get_type_str();
}

template<class T>
bool f$function_exists(const T &) {
  return true;
}


mixed f$error_get_last() {
  return {};
}

void f$warning(const string &message) {
  php_warning("%s", message.c_str());
}

template<class T>
int64_t f$get_reference_counter(const array<T> &v) {
  return v.get_reference_counter();
}

template<class T>
int64_t f$get_reference_counter(const class_instance<T> &v) {
  return v.get_reference_counter();
}

int64_t f$get_reference_counter(const string &v) {
  return v.get_reference_counter();
}

int64_t f$get_reference_counter(const mixed &v) {
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


template<class T>
typename array<T>::iterator begin(array<T> &x) {
  return x.begin();
}

template<class T>
typename array<T>::const_iterator begin(const array<T> &x) {
  return x.begin();
}

template<class T>
typename array<T>::const_iterator const_begin(const array<T> &x) {
  return x.begin();
}


array<mixed>::iterator begin(mixed &x) {
  return x.begin();
}

array<mixed>::const_iterator begin(const mixed &x) {
  return x.begin();
}

array<mixed>::const_iterator const_begin(const mixed &x) {
  return x.begin();
}


template<class T>
typename array<T>::iterator begin(Optional<array<T>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}

template<class T>
typename array<T>::const_iterator begin(const Optional<array<T>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}

template<class T>
typename array<T>::const_iterator const_begin(const Optional<array<T>> &x) {
  if (unlikely(!x.has_value())) {
    php_warning("Invalid argument supplied for foreach(), false or null is given");
  }
  return x.val().begin();
}


template<class T>
typename array<T>::iterator end(array<T> &x) {
  return x.end();
}

template<class T>
typename array<T>::const_iterator end(const array<T> &x) {
  return x.end();
}

template<class T>
typename array<T>::const_iterator const_end(const array<T> &x) {
  return x.end();
}

array<mixed>::iterator end(mixed &x) {
  return x.end();
}

array<mixed>::const_iterator end(const mixed &x) {
  return x.end();
}

array<mixed>::const_iterator const_end(const mixed &x) {
  return x.end();
}


template<class T>
typename array<T>::iterator end(Optional<array<T>> &x) {
  return x.val().end();
}

template<class T>
typename array<T>::const_iterator end(const Optional<array<T>> &x) {
  return x.val().end();
}

template<class T>
typename array<T>::const_iterator const_end(const Optional<array<T>> &x) {
  return x.val().end();
}


void clear_array(mixed &v) {
  v.clear();
}

template<class T>
void clear_array(array<T> &a) {
  a.clear();
}

template<class T>
void clear_array(Optional<array<T>> &a) {
  a = false;
}

template<class T>
void unset(array<T> &x) {
  x = {};
}

template<class T>
void unset(class_instance<T> &x) {
  x = {};
}

void unset(mixed &x) {
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
