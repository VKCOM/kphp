#pragma once

#include <tuple>
#include <utility>

#include "common/type_traits/list_of_types.h"

#include "runtime/allocator.h"
#include "runtime/include.h"
#include "runtime/kphp_type_traits.h"
#include "runtime/profiler.h"

// order of includes below matters, be careful

#define INCLUDED_FROM_KPHP_CORE

#include "string_decl.inl"
#include "array_decl.inl"
#include "class_instance_decl.inl"
#include "variable_decl.inl"
#include "string_buffer_decl.inl"

#include "string.inl"
#include "array.inl"
#include "class_instance.inl"
#include "variable.inl"
#include "string_buffer.inl"

#undef INCLUDED_FROM_KPHP_CORE

#define unimplemented_function(name, args...) ({                              \
  php_critical_error ("unimplemented_function: %s", f$strval (name).c_str()); \
  1;                                                                          \
})

#define require(flag_name, action) ({ \
  flag_name = true;                   \
  action;                             \
})

#define require_once(flag_name, action) ({ \
  if (!flag_name) {                        \
    require (flag_name, action);           \
  }                                        \
})

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

inline bool lt(const bool &lhs, const bool &rhs);

template<class T2>
inline bool lt(const bool &lhs, const T2 &rhs);

template<class T1>
inline bool lt(const T1 &lhs, const bool &rhs);

template<class T1, class T2>
inline bool lt(const T1 &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool lt(const OrFalse<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool lt(const T1 &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool lt(const OrFalse<T> &lhs, const OrFalse<T> &rhs);

template<class T1, class T2>
inline bool lt(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool lt(const bool &lhs, const OrFalse<T> &rhs);

template<class T>
inline bool lt(const OrFalse<T> &lhs, const bool &rhs);


inline bool gt(const bool &lhs, const bool &rhs);

template<class T2>
inline bool gt(const bool &lhs, const T2 &rhs);

template<class T1>
inline bool gt(const T1 &lhs, const bool &rhs);

template<class T1, class T2>
inline bool gt(const T1 &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool gt(const OrFalse<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool gt(const T1 &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool gt(const OrFalse<T> &lhs, const OrFalse<T> &rhs);

template<class T1, class T2>
inline bool gt(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool gt(const bool &lhs, const OrFalse<T> &rhs);

template<class T>
inline bool gt(const OrFalse<T> &lhs, const bool &rhs);


inline bool leq(const bool &lhs, const bool &rhs);

template<class T2>
inline bool leq(const bool &lhs, const T2 &rhs);

template<class T1>
inline bool leq(const T1 &lhs, const bool &rhs);

template<class T1, class T2>
inline bool leq(const T1 &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool leq(const OrFalse<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool leq(const T1 &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool leq(const OrFalse<T> &lhs, const OrFalse<T> &rhs);

template<class T1, class T2>
inline bool leq(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool leq(const bool &lhs, const OrFalse<T> &rhs);

template<class T>
inline bool leq(const OrFalse<T> &lhs, const bool &rhs);


inline bool geq(const bool &lhs, const bool &rhs);

template<class T2>
inline bool geq(const bool &lhs, const T2 &rhs);

template<class T1>
inline bool geq(const T1 &lhs, const bool &rhs);

template<class T1, class T2>
inline bool geq(const T1 &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool geq(const OrFalse<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool geq(const T1 &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool geq(const OrFalse<T> &lhs, const OrFalse<T> &rhs);

template<class T1, class T2>
inline bool geq(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs);

template<class T>
inline bool geq(const bool &lhs, const OrFalse<T> &rhs);

template<class T>
inline bool geq(const OrFalse<T> &lhs, const bool &rhs);


inline double divide(int lhs, int rhs);

inline double divide(double lhs, int rhs);

inline double divide(const string &lhs, int rhs);

inline double divide(const var &lhs, int rhs);


inline double divide(int lhs, double rhs);

inline double divide(double lhs, double rhs);

inline double divide(const string &lhs, double rhs);

inline double divide(const var &lhs, double rhs);


inline double divide(int lhs, const string &rhs);

inline double divide(double lhs, const string &rhs);

inline double divide(const string &lhs, const string &rhs);

inline double divide(const var &lhs, const string &rhs);


inline double divide(int lhs, const var &rhs);

inline double divide(double lhs, const var &rhs);

inline double divide(const string &lhs, const var &rhs);

inline double divide(const var &lhs, const var &rhs);


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
inline double divide(const OrFalse<T1> &lhs, const T2 &rhs); //not defined

template<class T1, class T2>
inline double divide(const T1 &lhs, const OrFalse<T2> &rhs); //not defined


inline int modulo(int lhs, int rhs);

template<class T1, class T2>
inline int modulo(const T1 &lhs, const T2 &rhs);

inline int int_power(int base, int exp) {
  int res = 1;
  while (exp > 0) {
    if (exp & 1) {
      res *= base;
    }
    base *= base;
    exp >>= 1;
  }
  return res;
}

inline double float_power(double base, int exp) {
  return std::pow(base, exp);
}

inline var var_power(const var &base, const var &exp) {
  if (base.is_int() && exp.is_int() && exp.to_int() >= 0) {
    return int_power(base.to_int(), exp.to_int());
  }
  const double float_base = base.to_float();
  if (exp.is_int()) {
    return std::pow(float_base, exp.to_int());
  }

  const double float_exp = exp.to_float();
  if (float_exp == static_cast<int>(float_exp)) {
    return std::pow(float_base, float_exp);
  }

  if (base < 0.0) {
    php_warning("Calculating pow with negative base and double exp will produce zero");
    return 0.0;
  }

  return std::pow(float_base, float_exp);
}

inline int &power_self(int &base, int exp) {
  return base = int_power(base, exp);
}

inline double &power_self(double &base, int exp) {
  return base = float_power(base, exp);
}

inline var &power_self(var &base, const var &exp) {
  return base = var_power(base, exp);
}

template<class T1, class T2>
inline T1 &divide_self(T1 &lhs, const T2 &rhs);


inline int &modulo_self(int &lhs, int rhs);

template<class T1, class T2>
inline T1 &modulo_self(T1 &lhs, const T2 &rhs);


template<class T0, class T>
inline void assign(T0 &dest, const T &from);


template<class T, class = enable_for_bool_int_double<T>>
inline bool f$boolval(const T &val);

inline bool f$boolval(const string &val);

template<class T>
inline bool f$boolval(const array<T> &val);

template<class T>
inline bool f$boolval(const class_instance<T> &val);

template<class ...Args>
bool f$boolval(const std::tuple<Args...> &val);

template<class T>
inline bool f$boolval(const OrFalse<T> &val);

inline bool f$boolval(const var &val);


template<class T, class = enable_for_bool_int_double<T>>
inline int f$intval(const T &val);

inline int f$intval(const string &val);

template<class T>
inline int f$intval(const array<T> &val);

template<class T>
inline int f$intval(const class_instance<T> &val);

inline int f$intval(const var &val);

template<class T>
inline int f$intval(const OrFalse<T> &val);


inline int f$safe_intval(const bool &val);

inline const int &f$safe_intval(const int &val);

inline int f$safe_intval(const double &val);

inline int f$safe_intval(const string &val);

template<class T>
inline int f$safe_intval(const array<T> &val);

template<class T>
inline int f$safe_intval(const class_instance<T> &val);

inline int f$safe_intval(const var &val);


template<class T, class = enable_for_bool_int_double<T>>
double f$floatval(const T &val);

inline double f$floatval(const string &val);

template<class T>
inline double f$floatval(const array<T> &val);

template<class T>
inline double f$floatval(const class_instance<T> &val);

inline double f$floatval(const var &val);

template<class T>
inline double f$floatval(const OrFalse<T> &val);


inline string f$strval(const bool &val);

inline string f$strval(const int &val);

inline string f$strval(const double &val);

inline string f$strval(const string &val);

template<class T>
inline string f$strval(const array<T> &val);

template<class ...Args>
inline string f$strval(const std::tuple<Args...> &val);

template<class T>
inline string f$strval(const OrFalse<T> &val);

template<class T>
inline string f$strval(const class_instance<T> &val);

inline string f$strval(const var &val);


template<class T>
inline array<T> f$arrayval(const T &val);

template<class T>
inline const array<T> &f$arrayval(const array<T> &val);

template<class T>
inline array<var> f$arrayval(const class_instance<T> &val);

inline array<var> f$arrayval(const var &val);

template<class T>
inline array<var> f$arrayval(const OrFalse<T> &val);


inline bool &boolval_ref(bool &val);

inline bool &boolval_ref(var &val);

inline const bool &boolval_ref(const bool &val);

inline const bool &boolval_ref(const var &val);


inline int &intval_ref(int &val);

inline int &intval_ref(var &val);

inline const int &intval_ref(const int &val);

inline const int &intval_ref(const var &val);


inline double &floatval_ref(double &val);

inline double &floatval_ref(var &val);

inline const double &floatval_ref(const double &val);

inline const double &floatval_ref(const var &val);


inline string &strval_ref(string &val);

inline string &strval_ref(var &val);

inline const string &strval_ref(const string &val);

inline const string &strval_ref(const var &val);


template<class T>
inline array<T> &arrayval_ref(array<T> &val, const char *function, int parameter_num);

inline array<var> &arrayval_ref(var &val, const char *function, int parameter_num);

template<class T>
inline const array<T> &arrayval_ref(const array<T> &val, const char *function, int parameter_num);

inline const array<var> &arrayval_ref(const var &val, const char *function, int parameter_num);


template<class T>
bool eq2(const OrFalse<T> &v, bool value);

template<class T>
bool eq2(bool value, const OrFalse<T> &v);

template<class T>
bool eq2(const OrFalse<T> &v, const OrFalse<T> &value);

template<class T, class T1>
bool eq2(const OrFalse<T> &v, const OrFalse<T1> &value);

template<class T, class T1>
bool eq2(const OrFalse<T> &v, const T1 &value);

template<class T, class T1>
bool eq2(const T1 &value, const OrFalse<T> &v);

template<class T>
bool equals(const OrFalse<T> &value, const OrFalse<T> &v);

template<class T, class T1>
bool equals(const OrFalse<T1> &value, const OrFalse<T> &v);

template<class T, class T1>
bool equals(const T1 &value, const OrFalse<T> &v);

template<class T, class T1>
bool equals(const OrFalse<T> &v, const T1 &value);


template<class T, class = enable_for_bool_int_double<T>>
inline bool f$empty(const T &v);
inline bool f$empty(const string &v);
inline bool f$empty(const var &v);
template<class T>
inline bool f$empty(const array<T> &);
template<class T>
inline bool f$empty(const class_instance<T> &);
template<class T>
inline bool f$empty(const OrFalse<T> &);


template<class T>
bool f$is_numeric(const T &);
template<class T>
inline bool f$is_numeric(const OrFalse<T> &v);
inline bool f$is_numeric(const string &v);
inline bool f$is_numeric(const var &v);

template<class T, class = enable_for_bool_int_double_array<T>>
inline bool f$is_null(const T &v);
template<class T>
inline bool f$is_null(const class_instance<T> &v);
template<class T>
inline bool f$is_null(const OrFalse<T> &v);
inline bool f$is_null(const var &v);

template<class T>
inline bool f$is_bool(const T &v);
template<class T>
inline bool f$is_bool(const OrFalse<T> &v);
inline bool f$is_bool(const var &v);

template<class T>
inline bool f$is_int(const T &);
template<class T>
inline bool f$is_int(const OrFalse<T> &v);
inline bool f$is_int(const var &v);

template<class T>
inline bool f$is_float(const T &v);
template<class T>
inline bool f$is_float(const OrFalse<T> &v);
inline bool f$is_float(const var &v);


template<class T>
inline bool f$is_scalar(const T &v);
template<class T>
inline bool f$is_scalar(const OrFalse<T> &v);
inline bool f$is_scalar(const var &v);


template<class T>
inline bool f$is_string(const T &v);
template<class T>
inline bool f$is_string(const OrFalse<T> &v);
inline bool f$is_string(const var &v);


template<class T>
inline bool f$is_array(const T &v);
template<class T>
inline bool f$is_array(const OrFalse<T> &v);
inline bool f$is_array(const var &v);


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

inline const char *get_type_c_str(const bool &v);
inline const char *get_type_c_str(const int &v);
inline const char *get_type_c_str(const double &v);
inline const char *get_type_c_str(const string &v);
inline const char *get_type_c_str(const var &v);
template<class T>
inline const char *get_type_c_str(const array<T> &v);
template<class T>
inline const char *get_type_c_str(const class_instance<T> &v);

template<class T>
inline string f$get_type(const T &v);

inline string f$get_class(const bool &v);
inline string f$get_class(const int &v);
inline string f$get_class(const double &v);
inline string f$get_class(const string &v);
inline string f$get_class(const var &v);
template<class T>
inline string f$get_class(const array<T> &v);
template<class T>
inline string f$get_class(const class_instance<T> &v);


inline int f$count(const var &v);

template<class T>
inline int f$count(const OrFalse<T> &a);

template<class T>
inline int f$count(const array<T> &a);

template<class ...Args>
inline int f$count(const std::tuple<Args...> &a);

template<class T>
inline int f$count(const T &v);


template<class T>
int f$sizeof(const T &v);


inline string &append(string &dest, const string &from);

template<class T>
inline string &append(OrFalse<string> &dest, const T &from);

template<class T>
inline string &append(string &dest, const T &from);

template<class T>
inline var &append(var &dest, const T &from);

template<class T0, class T>
inline T0 &append(T0 &dest, const T &from);


inline string f$gettype(const var &v);

template<class T>
inline bool f$function_exists(const T &a1);


const int E_ERROR = 1;
const int E_WARNING = 2;
const int E_PARSE = 4;
const int E_NOTICE = 8;
const int E_CORE_ERROR = 16;
const int E_CORE_WARNING = 32;
const int E_COMPILE_ERROR = 64;
const int E_COMPILE_WARNING = 128;
const int E_USER_ERROR = 256;
const int E_USER_WARNING = 512;
const int E_USER_NOTICE = 1024;
const int E_STRICT = 2048;
const int E_RECOVERABLE_ERROR = 4096;
const int E_DEPRECATED = 8192;
const int E_USER_DEPRECATED = 16384;
const int E_ALL = 32767;

inline var f$error_get_last();

inline int f$error_reporting(int level);

inline int f$error_reporting();

inline void f$warning(const string &message);

inline int f$memory_get_static_usage();

inline int f$memory_get_peak_usage(bool real_usage = false);

inline int f$memory_get_usage(bool real_usage = false);

inline int f$memory_get_total_usage();


template<class T>
inline int f$get_reference_counter(const array<T> &v);

template<class T>
inline int f$get_reference_counter(const class_instance<T> &v);

inline int f$get_reference_counter(const string &v);

inline int f$get_reference_counter(const var &v);


template<class T>
inline T &val(T &x);

template<class T>
inline const T &val(const T &x);

template<class T>
inline T &ref(T &x);

template<class T>
inline const T &val(const OrFalse<T> &x);

template<class T>
inline T &val(OrFalse<T> &x);

template<class T>
inline T &ref(OrFalse<T> &x);


template<class T>
inline typename array<T>::iterator begin(array<T> &x);

template<class T>
inline typename array<T>::const_iterator begin(const array<T> &x);

template<class T>
inline typename array<T>::const_iterator const_begin(const array<T> &x);

inline array<var>::iterator begin(var &x);

inline array<var>::const_iterator begin(const var &x);

inline array<var>::const_iterator const_begin(const var &x);

template<class T>
inline typename array<T>::iterator begin(OrFalse<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator begin(const OrFalse<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator const_begin(const OrFalse<array<T>> &x);

template<class T>
inline typename array<T>::iterator end(array<T> &x);

template<class T>
inline typename array<T>::const_iterator end(const array<T> &x);

template<class T>
inline typename array<T>::const_iterator const_end(const array<T> &x);

inline array<var>::iterator end(var &x);

inline array<var>::const_iterator end(const var &x);

inline array<var>::const_iterator const_end(const var &x);

template<class T>
inline typename array<T>::iterator end(OrFalse<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator end(const OrFalse<array<T>> &x);

template<class T>
inline typename array<T>::const_iterator const_end(const OrFalse<array<T>> &x);


inline void clear_array(var &v);

template<class T>
inline void clear_array(array<T> &a);

template<class T>
inline void clear_array(OrFalse<array<T>> &a);

template<class T>
inline void unset(array<T> &x);

template<class T>
inline void unset(class_instance<T> &x);

inline void unset(var &x);


/*
 *
 *     IMPLEMENTATION
 *
 */


bool lt(const bool &lhs, const bool &rhs) {
  return lhs < rhs;
}

template<class T2>
bool lt(const bool &lhs, const T2 &rhs) {
  return lhs < f$boolval(rhs);
}

template<class T1>
bool lt(const T1 &lhs, const bool &rhs) {
  return f$boolval(lhs) < rhs;
}

template<class T1, class T2>
bool lt(const T1 &lhs, const T2 &rhs) {
  return lhs < rhs;
}

template<class T1, class T2>
bool lt(const OrFalse<T1> &lhs, const T2 &rhs) {
  return lhs.bool_value ? lt(lhs.value, rhs) : lt(false, rhs);
}

template<class T1, class T2>
bool lt(const T1 &lhs, const OrFalse<T2> &rhs) {
  return rhs.bool_value ? lt(lhs, rhs.value) : lt(lhs, false);
}

template<class T>
bool lt(const OrFalse<T> &lhs, const OrFalse<T> &rhs) {
  return lhs.bool_value ? lt(lhs.value, rhs) : lt(false, rhs);
}

template<class T1, class T2>
bool lt(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs) {
  return lhs.bool_value ? lt(lhs.value, rhs) : lt(false, rhs);
}

template<class T>
bool lt(const bool &lhs, const OrFalse<T> &rhs) {
  return lt(lhs, f$boolval(rhs));
}

template<class T>
bool lt(const OrFalse<T> &lhs, const bool &rhs) {
  return lt(f$boolval(lhs), rhs);
}


bool gt(const bool &lhs, const bool &rhs) {
  return lhs > rhs;
}

template<class T2>
bool gt(const bool &lhs, const T2 &rhs) {
  return lhs > f$boolval(rhs);
}

template<class T1>
bool gt(const T1 &lhs, const bool &rhs) {
  return f$boolval(lhs) > rhs;
}

template<class T1, class T2>
bool gt(const T1 &lhs, const T2 &rhs) {
  return lhs > rhs;
}

template<class T1, class T2>
bool gt(const OrFalse<T1> &lhs, const T2 &rhs) {
  return lhs.bool_value ? gt(lhs.value, rhs) : gt(false, rhs);
}

template<class T1, class T2>
bool gt(const T1 &lhs, const OrFalse<T2> &rhs) {
  return rhs.bool_value ? gt(lhs, rhs.value) : gt(lhs, false);
}

template<class T>
bool gt(const OrFalse<T> &lhs, const OrFalse<T> &rhs) {
  return lhs.bool_value ? gt(lhs.value, rhs) : gt(false, rhs);
}

template<class T1, class T2>
bool gt(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs) {
  return lhs.bool_value ? gt(lhs.value, rhs) : gt(false, rhs);
}

template<class T>
bool gt(const bool &lhs, const OrFalse<T> &rhs) {
  return gt(lhs, f$boolval(rhs));
}

template<class T>
bool gt(const OrFalse<T> &lhs, const bool &rhs) {
  return gt(f$boolval(lhs), rhs);
}


bool leq(const bool &lhs, const bool &rhs) {
  return lhs <= rhs;
}

template<class T2>
bool leq(const bool &lhs, const T2 &rhs) {
  return lhs <= f$boolval(rhs);
}

template<class T1>
bool leq(const T1 &lhs, const bool &rhs) {
  return f$boolval(lhs) <= rhs;
}

template<class T1, class T2>
bool leq(const T1 &lhs, const T2 &rhs) {
  return lhs <= rhs;
}

template<class T1, class T2>
bool leq(const OrFalse<T1> &lhs, const T2 &rhs) {
  return lhs.bool_value ? leq(lhs.value, rhs) : leq(false, rhs);
}

template<class T1, class T2>
bool leq(const T1 &lhs, const OrFalse<T2> &rhs) {
  return rhs.bool_value ? leq(lhs, rhs.value) : leq(lhs, false);
}

template<class T>
bool leq(const OrFalse<T> &lhs, const OrFalse<T> &rhs) {
  return lhs.bool_value ? leq(lhs.value, rhs) : leq(false, rhs);
}

template<class T1, class T2>
bool leq(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs) {
  return lhs.bool_value ? leq(lhs.value, rhs) : leq(false, rhs);
}

template<class T>
bool leq(const bool &lhs, const OrFalse<T> &rhs) {
  return leq(lhs, f$boolval(rhs));
}

template<class T>
bool leq(const OrFalse<T> &lhs, const bool &rhs) {
  return leq(f$boolval(lhs), rhs);
}


bool geq(const bool &lhs, const bool &rhs) {
  return lhs >= rhs;
}

template<class T2>
bool geq(const bool &lhs, const T2 &rhs) {
  return lhs >= f$boolval(rhs);
}

template<class T1>
bool geq(const T1 &lhs, const bool &rhs) {
  return f$boolval(lhs) >= rhs;
}

template<class T1, class T2>
bool geq(const T1 &lhs, const T2 &rhs) {
  return lhs >= rhs;
}

template<class T1, class T2>
bool geq(const OrFalse<T1> &lhs, const T2 &rhs) {
  return lhs.bool_value ? geq(lhs.value, rhs) : geq(false, rhs);
}

template<class T1, class T2>
bool geq(const T1 &lhs, const OrFalse<T2> &rhs) {
  return rhs.bool_value ? geq(lhs, rhs.value) : geq(lhs, false);
}

template<class T>
bool geq(const OrFalse<T> &lhs, const OrFalse<T> &rhs) {
  return lhs.bool_value ? geq(lhs.value, rhs) : geq(false, rhs);
}

template<class T1, class T2>
bool geq(const OrFalse<T1> &lhs, const OrFalse<T2> &rhs) {
  return lhs.bool_value ? geq(lhs.value, rhs) : geq(false, rhs);
}

template<class T>
bool geq(const bool &lhs, const OrFalse<T> &rhs) {
  return geq(lhs, f$boolval(rhs));
}

template<class T>
bool geq(const OrFalse<T> &lhs, const bool &rhs) {
  return geq(f$boolval(lhs), rhs);
}


double divide(int lhs, int rhs) {
  if (rhs == 0) {
    php_warning("Integer division by zero");
    return 0;
  }

  return double(lhs) / rhs;
}

double divide(double lhs, int rhs) {
  if (rhs == 0) {
    php_warning("Integer division by zero");
    return 0;
  }

  return lhs / rhs;
}

double divide(const string &lhs, int rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const var &lhs, int rhs) {
  return divide(f$floatval(lhs), rhs);
}


double divide(int lhs, double rhs) {
  if (rhs == 0) {
    php_warning("Float division by zero");
    return 0;
  }

  return lhs / rhs;
}

double divide(double lhs, double rhs) {
  if (rhs == 0) {
    php_warning("Float division by zero");
    return 0;
  }

  return lhs / rhs;
}

double divide(const string &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const var &lhs, double rhs) {
  return divide(f$floatval(lhs), rhs);
}


double divide(int lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(double lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(const string &lhs, const string &rhs) {
  return divide(f$floatval(lhs), f$floatval(rhs));
}

double divide(const var &lhs, const string &rhs) {
  return divide(lhs, f$floatval(rhs));
}


double divide(int lhs, const var &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(double lhs, const var &rhs) {
  return divide(lhs, f$floatval(rhs));
}

double divide(const string &lhs, const var &rhs) {
  return divide(f$floatval(lhs), rhs);
}

double divide(const var &lhs, const var &rhs) {
  return f$floatval(lhs / rhs);
}


double divide(bool lhs, bool) {
  php_warning("Both arguments of operator '/' are bool");
  return lhs;
}

template<class T>
double divide(bool lhs, const T &rhs) {
  php_warning("First argument of operator '/' is bool");
  return divide((int)lhs, f$floatval(rhs));
}

template<class T>
double divide(const T &lhs, bool) {
  php_warning("Second argument of operator '/' is bool");
  return f$floatval(lhs);
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


int modulo(int lhs, int rhs) {
  if (rhs == 0) {
    php_warning("Modulo by zero");
    return 0;
  }
  return lhs % rhs;
}

template<class T1, class T2>
int modulo(const T1 &lhs, const T2 &rhs) {
  int div = f$intval(lhs);
  int mod = f$intval(rhs);

  if (neq2(div, lhs)) {
    php_warning("First parameter of operator %% is not an integer");
  }
  if (neq2(mod, rhs)) {
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


int &modulo_self(int &lhs, int rhs) {
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

template<class T, class>
bool f$boolval(const T &val) {
  return static_cast<bool>(val);
}

bool f$boolval(const string &val) {
  return val.to_bool();
}

template<class T>
bool f$boolval(const array<T> &val) {
  return !val.empty();
}

template<class T>
bool f$boolval(const class_instance<T> &val) {
  return !val.is_null();
}

template<class ...Args>
bool f$boolval(const std::tuple<Args...> &) {
  return true;
}

template<class T>
bool f$boolval(const OrFalse<T> &val) {
  return val.bool_value ? f$boolval(val.val()) : false;
}

bool f$boolval(const var &val) {
  return val.to_bool();
}


template<class T, class>
int f$intval(const T &val) {
  return static_cast<int>(val);
}

int f$intval(const string &val) {
  return val.to_int();
}

template<class T>
int f$intval(const array<T> &val) {
  php_warning("Wrong convertion from array to int");
  return val.to_int();
}

template<class T>
int f$intval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to int");
  return 1;
}

int f$intval(const var &val) {
  return val.to_int();
}

template<class T>
inline int f$intval(const OrFalse<T> &val) {
  return val.bool_value ? f$intval(val.val()) : 0;
}


int f$safe_intval(const bool &val) {
  return val;
}

const int &f$safe_intval(const int &val) {
  return val;
}

int f$safe_intval(const double &val) {
  if (fabs(val) > 2147483648) {
    php_warning("Wrong convertion from double %.6lf to int", val);
  }
  return (int)val;
}

int f$safe_intval(const string &val) {
  return val.safe_to_int();
}

template<class T>
int f$safe_intval(const array<T> &val) {
  php_warning("Wrong convertion from array to int");
  return val.to_int();
}

template<class T>
int f$safe_intval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to int");
  return 1;
}

int f$safe_intval(const var &val) {
  return val.safe_to_int();
}


template<class T, class>
double f$floatval(const T &val) {
  return static_cast<double>(val);
}

double f$floatval(const string &val) {
  return val.to_float();
}

template<class T>
double f$floatval(const array<T> &val) {
  php_warning("Wrong convertion from array to float");
  return val.to_float();
}

template<class T>
double f$floatval(const class_instance<T> &) {
  php_warning("Wrong convertion from object to float");
  return 1.0;
}

double f$floatval(const var &val) {
  return val.to_float();
}

template<class T>
inline double f$floatval(const OrFalse<T> &val) {
  return val.bool_value ? f$floatval(val.val()) : 0;
}


string f$strval(const bool &val) {
  return (val ? string("1", 1) : string());
}

string f$strval(const int &val) {
  return string(val);
}

string f$strval(const double &val) {
  return string(val);
}

string f$strval(const string &val) {
  return val;
}

template<class T>
string f$strval(const array<T> &) {
  php_warning("Convertion from array to string");
  return string("Array", 5);
}

template<class ...Args>
string f$strval(const std::tuple<Args...> &) {
  php_warning("Convertion from tuple to string");
  return string("Array", 5);
}

template<class T>
string f$strval(const OrFalse<T> &val) {
  return val.bool_value ? f$strval(val.val()) : f$strval(false);
}

template<class T>
string f$strval(const class_instance<T> &val) {
  return val.to_string();
}

string f$strval(const var &val) {
  return val.to_string();
}


template<class T>
array<T> f$arrayval(const T &val) {
  array<T> res(array_size(1, 0, true));
  res.push_back(val);
  return res;
}

template<class T>
const array<T> &f$arrayval(const array<T> &val) {
  return val;
}

template<class T>
array<var> f$arrayval(const class_instance<T> &) {
  php_warning("Can not convert class instance to array");
  return array<var>();
}

array<var> f$arrayval(const var &val) {
  return val.to_array();
}

template<class T>
inline array<var> f$arrayval(const OrFalse<T> &val) {
  return val.bool_value ? f$arrayval(val.val()) : f$arrayval(false);
}

bool &boolval_ref(bool &val) {
  return val;
}

bool &boolval_ref(var &val) {
  return val.as_bool("unknown", -1);
}

const bool &boolval_ref(const bool &val) {
  return val;
}

const bool &boolval_ref(const var &val) {
  return val.as_bool("unknown", -1);
}


int &intval_ref(int &val) {
  return val;
}

int &intval_ref(var &val) {
  return val.as_int("unknown", -1);
}

const int &intval_ref(const int &val) {
  return val;
}

const int &intval_ref(const var &val) {
  return val.as_int("unknown", -1);
}


double &floatval_ref(double &val) {
  return val;
}

double &floatval_ref(var &val) {
  return val.as_float("unknown", -1);
}

const double &floatval_ref(const double &val) {
  return val;
}

const double &floatval_ref(const var &val) {
  return val.as_float("unknown", -1);
}


string &strval_ref(string &val) {
  return val;
}

string &strval_ref(var &val) {
  return val.as_string("unknown", -1);
}

const string &strval_ref(const string &val) {
  return val;
}

const string &strval_ref(const var &val) {
  return val.as_string("unknown", -1);
}


template<class T>
array<T> &arrayval_ref(array<T> &val, const char *function __attribute__((unused)), int parameter_num __attribute__((unused))) {
  return val;
}

array<var> &arrayval_ref(var &val, const char *function, int parameter_num) {
  return val.as_array(function, parameter_num);
}

template<class T>
const array<T> &arrayval_ref(const array<T> &val, const char *function __attribute__((unused)), int parameter_num __attribute__((unused))) {
  return val;
}

const array<var> &arrayval_ref(const var &val, const char *function, int parameter_num) {
  return val.as_array(function, parameter_num);
}


template<class T>
bool eq2(const OrFalse<T> &v, bool value) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T>
bool eq2(bool value, const OrFalse<T> &v) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T>
bool eq2(const OrFalse<T> &v, const OrFalse<T> &value) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T, class T1>
bool eq2(const OrFalse<T> &v, const OrFalse<T1> &value) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T, class T1>
bool eq2(const OrFalse<T> &v, const T1 &value) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T, class T1>
bool eq2(const T1 &value, const OrFalse<T> &v) {
  return likely (v.bool_value) ? eq2(v.value, value) : eq2(false, value);
}

template<class T>
bool equals(const OrFalse<T> &value, const OrFalse<T> &v) {
  return likely (v.bool_value) ? equals(v.value, value) : equals(false, value);
}

template<class T, class T1>
bool equals(const OrFalse<T1> &value, const OrFalse<T> &v) {
  return likely (v.bool_value) ? equals(v.value, value) : equals(false, value);
}

template<class T, class T1>
bool equals(const T1 &value, const OrFalse<T> &v) {
  return likely (v.bool_value) ? equals(v.value, value) : equals(false, value);
}

template<class T, class T1>
bool equals(const OrFalse<T> &v, const T1 &value) {
  return likely (v.bool_value) ? equals(v.value, value) : equals(false, value);
}


template<class T>
const T &convert_to<T>::convert(const T &val) {
  return val;
}

template<class T>
T convert_to<T>::convert(const Unknown &) {
  return T();
}

template<class T>
template<class T1>
T convert_to<T>::convert(const T1 &val) {
  return val;
}

template<>
template<class T1>
bool convert_to<bool>::convert(const T1 &val) {
  return f$boolval(val);
}

template<>
template<class T1>
int convert_to<int>::convert(const T1 &val) {
  return f$intval(val);
}

template<>
template<class T1>
double convert_to<double>::convert(const T1 &val) {
  return f$floatval(val);
}

template<>
template<class T1>
string convert_to<string>::convert(const T1 &val) {
  return f$strval(val);
}

template<>
template<class T1>
array<var> convert_to<array<var>>::convert(const T1 &val) {
  return f$arrayval(val);
}

template<>
template<class T1>
var convert_to<var>::convert(const T1 &val) {
  return var(val);
}


template<class T, class>
inline bool f$empty(const T &v) {
  return v == 0;
}

bool f$empty(const string &v) {
  int l = v.size();
  return l == 0 || (l == 1 && v[0] == '0');
}

template<class T>
bool f$empty(const array<T> &a) {
  return a.empty();
}

template<class T>
bool f$empty(const class_instance<T> &) {
  return false;
}

bool f$empty(const var &v) {
  return v.empty();
}

template<class T>
bool f$empty(const OrFalse<T> &a) {
  return a.bool_value ? f$empty(a.val()) : true;
}


int f$count(const var &v) {
  return v.count();
}

template<class T>
int f$count(const OrFalse<T> &a) {
  return a.bool_value ? f$count(a.val()) : f$count(false);
}

template<class T>
int f$count(const array<T> &a) {
  return a.count();
}

template<class ...Args>
inline int f$count(const std::tuple<Args...> &a __attribute__ ((unused))) {
  return (int)std::tuple_size<std::tuple<Args...>>::value;
}

template<class T>
int f$count(const T &) {
  php_warning("Count on non-array");
  return 1;
}


template<class T>
int f$sizeof(const T &v) {
  return f$count(v);
}


template<class T>
bool f$is_numeric(const T &) {
  return std::is_same<T, int>::value || std::is_same<T, double>::value;
}

bool f$is_numeric(const string &v) {
  return v.is_numeric();
}

bool f$is_numeric(const var &v) {
  return v.is_numeric();
}

template<class T>
inline bool f$is_numeric(const OrFalse<T> &v) {
  return v.bool_value ? f$is_numeric(v.val()) : false;
}


template<class T, class>
bool f$is_null(const T &) {
  return false;
}

template<class T>
bool f$is_null(const class_instance<T> &v) {
  return v.is_null();
}

template<class T>
inline bool f$is_null(const OrFalse<T> &v) {
  return v.bool_value ? f$is_null(v.val()) : false;
}

bool f$is_null(const var &v) {
  return v.is_null();
}


template<class T>
bool f$is_bool(const T &) {
  return std::is_same<T, bool>::value;
}

template<class T>
inline bool f$is_bool(const OrFalse<T> &v) {
  return v.bool_value ? f$is_bool(v.val()) : true;
}

bool f$is_bool(const var &v) {
  return v.is_bool();
}


template<class T>
bool f$is_int(const T &) {
  return std::is_same<T, int>::value;
}

template<class T>
inline bool f$is_int(const OrFalse<T> &v) {
  return v.bool_value ? f$is_int(v.val()) : false;
}

bool f$is_int(const var &v) {
  return v.is_int();
}


template<class T>
bool f$is_float(const T &) {
  return std::is_same<T, double>::value;
}

template<class T>
inline bool f$is_float(const OrFalse<T> &v) {
  return v.bool_value ? f$is_float(v.val()) : false;
}

bool f$is_float(const var &v) {
  return v.is_float();
}


template<class T>
bool f$is_scalar(const T &) {
  return std::is_arithmetic<T>::value || std::is_same<T, string>::value;
}

template<class T>
inline bool f$is_scalar(const OrFalse<T> &v) {
  return v.bool_value ? f$is_scalar(v.val()) : true;
}

bool f$is_scalar(const var &v) {
  return v.is_scalar();
}


template<class T>
bool f$is_string(const T &) {
  return std::is_same<T, string>::value;
}

template<class T>
inline bool f$is_string(const OrFalse<T> &v) {
  return v.bool_value ? f$is_string(v.val()) : false;
}

bool f$is_string(const var &v) {
  return v.is_string();
}


template<class T>
inline bool f$is_array(const T &) {
  return std::is_base_of<array_tag, T>::value;
}

bool f$is_array(const var &v) {
  return v.is_array();
}

template<class T>
inline bool f$is_array(const OrFalse<T> &v) {
  return v.bool_value ? f$is_array(v.val()) : false;
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

const char *get_type_c_str(const bool &) {
  return "boolean";
}

const char *get_type_c_str(const int &) {
  return "integer";
}

const char *get_type_c_str(const double &) {
  return "double";
}

const char *get_type_c_str(const string &) {
  return "string";
}

const char *get_type_c_str(const var &v) {
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
  return string(res, strlen(res));
}


string f$get_class(const bool &) {
  php_warning("Called get_class() on boolean");
  return string();
}

string f$get_class(const int &) {
  php_warning("Called get_class() on integer");
  return string();
}

string f$get_class(const double &) {
  php_warning("Called get_class() on double");
  return string();
}

string f$get_class(const string &) {
  php_warning("Called get_class() on string");
  return string();
}

string f$get_class(const var &v) {
  php_warning("Called get_class() on %s", v.get_type_c_str());
  return string();
}

template<class T>
string f$get_class(const array<T> &) {
  php_warning("Called get_class() on array");
  return string();
}

template<class T>
string f$get_class(const class_instance<T> &v) {
  const char *result = v.get_class();
  return string(result, (dl::size_type)strlen(result));
}


string &append(string &dest, const string &from) {
  return dest.append(from);
}

template<class T>
string &append(OrFalse<string> &dest, const T &from) {
  return append(dest.ref(), from);
}

template<class T>
string &append(string &dest, const T &from) {
  return dest.append(f$strval(from));
}

template<class T>
var &append(var &dest, const T &from) {
  return dest.append(f$strval(from));
}

template<class T0, class T>
T0 &append(T0 &dest, const T &from) {
  php_warning("Wrong arguments types %s and %s for operator .=", get_type_c_str(dest), get_type_c_str(from));
  return dest;
}


string f$gettype(const var &v) {
  return v.get_type();
}

template<class T>
bool f$function_exists(const T &) {
  return true;
}


var f$error_get_last() {
  return var();
}

int f$error_reporting(int level) {
  int prev = php_warning_level;
  if ((level & E_ALL) == E_ALL) {
    php_warning_level = 3;
  }
  if (0 <= level && level <= 3) {
    php_warning_level = level;
  }
  return prev;
}

int f$error_reporting() {
  return php_warning_level;
}

void f$warning(const string &message) {
  php_warning("%s", message.c_str());
}

int f$memory_get_static_usage() {
  return (int)dl::static_memory_used;
}

int f$memory_get_peak_usage(bool real_usage) {
  if (real_usage) {
    return (int)dl::get_script_memory_stats().max_real_memory_used;
  } else {
    return (int)dl::get_script_memory_stats().max_memory_used;
  }
}

int f$memory_get_usage(bool real_usage __attribute__((unused))) {
  return (int)dl::get_script_memory_stats().memory_used;
}

int f$memory_get_total_usage() {
  return (int)dl::get_script_memory_stats().real_memory_used;
}


template<class T>
int f$get_reference_counter(const array<T> &v) {
  return v.get_reference_counter();
}

template<class T>
int f$get_reference_counter(const class_instance<T> &v) {
  return v.get_reference_counter();
}

int f$get_reference_counter(const string &v) {
  return v.get_reference_counter();
}

int f$get_reference_counter(const var &v) {
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
const T &val(const OrFalse<T> &x) {
  return x.val();
}

template<class T>
T &val(OrFalse<T> &x) {
  return x.val();
}

template<class T>
T &ref(OrFalse<T> &x) {
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


array<var>::iterator begin(var &x) {
  return x.begin();
}

array<var>::const_iterator begin(const var &x) {
  return x.begin();
}

array<var>::const_iterator const_begin(const var &x) {
  return x.begin();
}


template<class T>
typename array<T>::iterator begin(OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.begin();
}

template<class T>
typename array<T>::const_iterator begin(const OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.begin();
}

template<class T>
typename array<T>::const_iterator const_begin(const OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.begin();
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

array<var>::iterator end(var &x) {
  return x.end();
}

array<var>::const_iterator end(const var &x) {
  return x.end();
}

array<var>::const_iterator const_end(const var &x) {
  return x.end();
}


template<class T>
typename array<T>::iterator end(OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.end();
}

template<class T>
typename array<T>::const_iterator end(const OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.end();
}

template<class T>
typename array<T>::const_iterator const_end(const OrFalse<array<T>> &x) {
  if (!x.bool_value) {
    php_warning("Invalid argument supplied for foreach(), false is given");
  }
  return x.value.end();
}


void clear_array(var &v) {
  v.clear();
}

template<class T>
void clear_array(array<T> &a) {
  a.clear();
}

template<class T>
void clear_array(OrFalse<array<T>> &a) {
  a.value.clear();
  a.bool_value = false;
}

template<class T>
void unset(array<T> &x) {
  clear_array(x);
}

template<class T>
void unset(class_instance<T> &x) {
  x.destroy();
}

void unset(var &x) {
  x = var();
}

