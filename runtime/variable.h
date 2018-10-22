#pragma once

/*
 *
 *   Do not include with file directly
 *   Include kphp_core.h instead
 *
 */

#include "runtime/string_buffer.h"

class var {
  enum var_type {
    NULL_TYPE,
    BOOLEAN_TYPE,
    INTEGER_TYPE,
    FLOAT_TYPE,
    STRING_TYPE,
    ARRAY_TYPE
  };

  var_type type;

  union {
    bool b;
    int i;
    double f;
    string s;
    array<var> a;
  };

  inline void copy_from(const var &other);
  inline void copy_from(var &&other);

public:
  var(const void *) = delete; // deprecate conversion from pointer to boolean
  inline var();
  inline var(const Unknown &u);
  inline var(bool b);
  inline var(int i);
  inline var(double f);
  inline var(const string &s);
  inline var(const char *s, int len);
  template<class T, class = enable_if_constructible_or_unknown<var, T>>
  inline var(const array<T> &a);
  inline var(const var &v);
  inline var(var &&v);

  inline var(const OrFalse<int> &v);
  inline var(const OrFalse<double> &v);
  inline var(const OrFalse<string> &v);
  template<class T, class = enable_if_constructible_or_unknown<var, T>>
  inline var(const OrFalse<array<T>> &v);

  inline var &operator=(bool other);
  inline var &operator=(int other);
  inline var &operator=(double other);
  inline var &operator=(const string &other);
  inline var &assign(const char *other, int len);
  template<class T, class = enable_if_constructible_or_unknown<var, T>>
  inline var &operator=(const array<T> &other);
  inline var &operator=(const var &other);
  inline var &operator=(var &&other);

  inline var &operator=(const OrFalse<int> &other);
  inline var &operator=(const OrFalse<double> &other);
  inline var &operator=(const OrFalse<string> &other);
  template<class T, class = enable_if_constructible_or_unknown<var, T>>
  inline var &operator=(const OrFalse<array<T>> &other);

  inline const var operator-() const;
  inline const var operator+() const;

  inline int operator~() const;

  inline var &operator+=(const var &other);
  inline var &operator-=(const var &other);
  inline var &operator*=(const var &other);
  inline var &operator/=(const var &other);
  inline var &operator%=(const var &other);

  inline var &operator&=(const var &other);
  inline var &operator|=(const var &other);
  inline var &operator^=(const var &other);
  inline var &operator<<=(const var &other);
  inline var &operator>>=(const var &other);

  inline var &safe_set_add(const var &other);
  inline var &safe_set_sub(const var &other);
  inline var &safe_set_mul(const var &other);
  inline var &safe_set_shl(const var &other);


  inline var &operator++();
  inline const var operator++(int);

  inline var &operator--();
  inline const var operator--(int);


  inline var &safe_incr_pre();
  inline const var safe_incr_post();

  inline var &safe_decr_pre();
  inline const var safe_decr_post();


  inline bool operator!() const;

  inline var &append(const string &v);

  inline var &operator[](int int_key);
  inline var &operator[](const string &string_key);
  inline var &operator[](const var &v);
  inline var &operator[](const array<var>::const_iterator &it);
  inline var &operator[](const array<var>::iterator &it);

  inline void set_value(const int int_key, const var &v);
  inline void set_value(const string &string_key, const var &v);
  inline void set_value(const string &string_key, const var &v, int precomuted_hash);
  inline void set_value(const var &v, const var &value);
  inline void set_value(const array<var>::const_iterator &it);
  inline void set_value(const array<var>::iterator &it);

  inline const var get_value(const int int_key) const;
  inline const var get_value(const string &string_key) const;
  inline const var get_value(const string &string_key, int precomuted_hash) const;
  inline const var get_value(const var &v) const;
  inline const var get_value(const array<var>::const_iterator &it) const;
  inline const var get_value(const array<var>::iterator &it) const;

  inline void push_back(const var &v);
  inline const var push_back_return(const var &v);

  inline bool isset(int int_key) const;
  inline bool isset(const string &string_key) const;
  inline bool isset(const var &v) const;
  inline bool isset(const array<var>::const_iterator &it) const;
  inline bool isset(const array<var>::iterator &it) const;

  inline void unset(int int_key);
  inline void unset(const string &string_key);
  inline void unset(const var &v);
  inline void unset(const array<var>::const_iterator &it);
  inline void unset(const array<var>::iterator &it);

  inline void destroy();
  inline ~var();

  inline void clear();

  inline const var to_numeric() const;
  inline bool to_bool() const;
  inline int to_int() const;
  inline double to_float() const;
  inline const string to_string() const;
  inline const array<var> to_array() const;

  inline int safe_to_int() const;

  inline void convert_to_numeric();
  inline void convert_to_bool();
  inline void convert_to_int();
  inline void convert_to_float();
  inline void convert_to_string();

  inline void safe_convert_to_int();

  inline const bool &as_bool(const char *function, int parameter_num) const;
  inline const int &as_int(const char *function, int parameter_num) const;
  inline const double &as_float(const char *function, int parameter_num) const;
  inline const string &as_string(const char *function, int parameter_num) const;
  inline const array<var> &as_array(const char *function, int parameter_num) const;

  inline bool &as_bool(const char *function, int parameter_num);
  inline int &as_int(const char *function, int parameter_num);
  inline double &as_float(const char *function, int parameter_num);
  inline string &as_string(const char *function, int parameter_num);
  inline array<var> &as_array(const char *function, int parameter_num);

  inline bool is_numeric() const;
  inline bool is_scalar() const;

  inline bool is_null() const;
  inline bool is_bool() const;
  inline bool is_int() const;
  inline bool is_float() const;
  inline bool is_string() const;
  inline bool is_array() const;

  inline const string get_type() const;
  inline const char *get_type_c_str() const;

  inline bool empty() const;
  inline int count() const;

  inline array<var>::const_iterator begin() const;
  inline array<var>::const_iterator end() const;

  inline array<var>::iterator begin();
  inline array<var>::iterator end();

  inline void swap(var &other);

  inline int get_reference_counter() const;
  inline void set_reference_counter_to_const();

  inline friend const var operator-(const string &lhs);

  inline friend const var operator+(const var &lhs, const var &rhs);
  inline friend const var operator-(const var &lhs, const var &rhs);
  inline friend const var operator*(const var &lhs, const var &rhs);
  inline friend const var operator/(const var &lhs, const var &rhs);
  inline friend const var operator%(const var &lhs, const var &rhs);

  inline friend const var safe_add(const var &lhs, const var &rhs);
  inline friend const var safe_sub(const var &lhs, const var &rhs);
  inline friend const var safe_mul(const var &lhs, const var &rhs);

  inline friend bool eq2(const var &lhs, const var &rhs);
  inline friend bool neq2(const var &lhs, const var &rhs);
  inline friend bool operator<=(const var &lhs, const var &rhs);
  inline friend bool operator>=(const var &lhs, const var &rhs);
  inline friend bool operator<(const var &lhs, const var &rhs);
  inline friend bool operator>(const var &lhs, const var &rhs);
  inline friend bool equals(const var &lhs, const var &rhs);
  inline friend bool not_equals(const var &lhs, const var &rhs);

  inline friend bool eq2(bool lhs, const var &rhs);
  inline friend bool eq2(int lhs, const var &rhs);
  inline friend bool eq2(double lhs, const var &rhs);
  inline friend bool eq2(const string &lhs, const var &rhs);
  template<class T>
  inline friend bool eq2(const array<T> &lhs, const var &rhs);
  template<class T>
  inline friend bool eq2(const var &lhs, bool rhs);
  inline friend bool eq2(const var &lhs, int rhs);
  inline friend bool eq2(const var &lhs, double rhs);
  inline friend bool eq2(const var &lhs, const string &rhs);
  template<class T>
  inline friend bool eq2(const var &lhs, const array<T> &rhs);

  inline friend bool equals(bool lhs, const var &rhs);
  inline friend bool equals(int lhs, const var &rhs);
  inline friend bool equals(double lhs, const var &rhs);
  inline friend bool equals(const string &lhs, const var &rhs);
  template<class T>
  inline friend bool equals(const array<T> &lhs, const var &rhs);
  inline friend bool equals(const var &lhs, bool rhs);
  inline friend bool equals(const var &lhs, int rhs);
  inline friend bool equals(const var &lhs, double rhs);
  inline friend bool equals(const var &lhs, const string &rhs);
  template<class T>
  inline friend bool equals(const var &lhs, const array<T> &rhs);


  friend void do_print_r(const var &v, int depth);
  friend void do_var_dump(const var &v, int depth);
  friend void do_var_export(const var &v, int depth, char endc);
  friend void do_serialize(const var &v);
  friend bool do_json_encode(const var &v, int options, bool simple_encode);
  friend dl::size_type max_string_size(const var &v);

  friend class string;

  friend inline string_buffer &operator<<(string_buffer &sb, const var &v);

  template<class T>
  friend class array;

  template<class T, class TT>
  friend
  class force_convert_to;
};

void do_var_export(const var &v, int depth, char endc = 0);

inline const var operator-(const string &lhs);
inline const var operator+(const string &lhs);

inline const var operator+(const var &lhs, const var &rhs);
inline const var operator-(const var &lhs, const var &rhs);
inline const var operator*(const var &lhs, const var &rhs);
inline const var operator/(const var &lhs, const var &rhs);
inline const var operator%(const var &lhs, const var &rhs);

inline int operator&(const var &lhs, const var &rhs);
inline int operator|(const var &lhs, const var &rhs);
inline int operator^(const var &lhs, const var &rhs);
inline int operator<<(const var &lhs, const var &rhs);
inline int operator>>(const var &lhs, const var &rhs);

inline const var safe_add(const var &lhs, const var &rhs);
inline const var safe_sub(const var &lhs, const var &rhs);
inline const var safe_mul(const var &lhs, const var &rhs);
inline const var safe_shl(const var &lhs, const var &rhs);


inline bool eq2(const var &lhs, const var &rhs);
inline bool neq2(const var &lhs, const var &rhs);
inline bool operator<=(const var &lhs, const var &rhs);
inline bool operator>=(const var &lhs, const var &rhs);
inline bool operator<(const var &lhs, const var &rhs);
inline bool operator>(const var &lhs, const var &rhs);
inline bool equals(const var &lhs, const var &rhs);
inline bool not_equals(const var &lhs, const var &rhs);


inline void swap(var &lhs, var &rhs);


template<class T, class T1>
inline array<T> &safe_set_add(array<T> &lhs, const array<T1> &rhs);

template<class T>
inline array<T> safe_add(const array<T> &lhs, const array<T> &rhs);


inline var &safe_set_add(var &lhs, const var &rhs);
inline var &safe_set_sub(var &lhs, const var &rhs);
inline var &safe_set_mul(var &lhs, const var &rhs);
inline var &safe_set_shl(var &lhs, const var &rhs);

inline int &safe_set_add(int &lhs, int rhs);
inline int &safe_set_sub(int &lhs, int rhs);
inline int &safe_set_mul(int &lhs, int rhs);
inline int &safe_set_shl(int &lhs, int rhs);

inline int safe_add(int lhs, int rhs);
inline int safe_sub(int lhs, int rhs);
inline int safe_mul(int lhs, int rhs);
inline int safe_shl(int lhs, int rhs);


inline double &safe_set_add(double &lhs, double rhs);
inline double &safe_set_sub(double &lhs, double rhs);
inline double &safe_set_mul(double &lhs, double rhs);

inline int safe_add(bool lhs, bool rhs);
inline int safe_add(bool lhs, int rhs);
inline double safe_add(bool lhs, double rhs);
inline int safe_add(int lhs, bool rhs);
inline double safe_add(int lhs, double rhs);
inline double safe_add(double lhs, bool rhs);
inline double safe_add(double lhs, int rhs);
inline double safe_add(double lhs, double rhs);

inline int safe_sub(bool lhs, bool rhs);
inline int safe_sub(bool lhs, int rhs);
inline double safe_sub(bool lhs, double rhs);
inline int safe_sub(int lhs, bool rhs);
inline double safe_sub(int lhs, double rhs);
inline double safe_sub(double lhs, bool rhs);
inline double safe_sub(double lhs, int rhs);
inline double safe_sub(double lhs, double rhs);

inline int safe_mul(bool lhs, bool rhs);
inline int safe_mul(bool lhs, int rhs);
inline double safe_mul(bool lhs, double rhs);
inline int safe_mul(int lhs, bool rhs);
inline double safe_mul(int lhs, double rhs);
inline double safe_mul(double lhs, bool rhs);
inline double safe_mul(double lhs, int rhs);
inline double safe_mul(double lhs, double rhs);


inline int &safe_incr_pre(int &lhs);
inline int &safe_decr_pre(int &lhs);
inline int safe_incr_post(int &lhs);
inline int safe_decr_post(int &lhs);

inline double &safe_incr_pre(double &lhs);
inline double &safe_decr_pre(double &lhs);
inline double safe_incr_post(double &lhs);
inline double safe_decr_post(double &lhs);

inline var &safe_incr_pre(var &lhs);
inline var &safe_decr_pre(var &lhs);
inline var safe_incr_post(var &lhs);
inline var safe_decr_post(var &lhs);


inline bool eq2(bool lhs, bool rhs);

inline bool eq2(int lhs, int rhs);

inline bool eq2(double lhs, double rhs);

inline bool eq2(bool lhs, int rhs);

inline bool eq2(bool lhs, double rhs);

inline bool eq2(int lhs, bool rhs);

inline bool eq2(double lhs, bool rhs);

inline bool eq2(int lhs, double rhs);

inline bool eq2(double lhs, int rhs);


inline bool eq2(bool lhs, const string &rhs);

inline bool eq2(int lhs, const string &rhs);

inline bool eq2(double lhs, const string &rhs);

inline bool eq2(const string &lhs, bool rhs);

inline bool eq2(const string &lhs, int rhs);

inline bool eq2(const string &lhs, double rhs);

template<class T>
inline bool eq2(bool lhs, const array<T> &rhs);

template<class ...Args>
inline bool eq2(bool lhs, const tuple<Args...> &rhs);

template<class T>
inline bool eq2(int lhs, const array<T> &rhs);

template<class T>
inline bool eq2(double lhs, const array<T> &rhs);

template<class T>
inline bool eq2(const string &lhs, const array<T> &rhs);

template<class T>
inline bool eq2(const array<T> &lhs, bool rhs);

template<class ...Args>
inline bool eq2(const tuple<Args...> &lhs, bool rhs);

template<class T>
inline bool eq2(const array<T> &lhs, int rhs);

template<class T>
inline bool eq2(const array<T> &lhs, double rhs);

template<class T>
inline bool eq2(const array<T> &lhs, const string &rhs);


template<class T>
inline bool eq2(bool lhs, const class_instance<T> &rhs);

template<class T>
inline bool eq2(int lhs, const class_instance<T> &rhs);

template<class T>
inline bool eq2(double lhs, const class_instance<T> &rhs);

template<class T>
inline bool eq2(const string &lhs, const class_instance<T> &rhs);

template<class T, class T1>
inline bool eq2(const array<T1> &lhs, const class_instance<T> &rhs);

template<class T>
inline bool eq2(const class_instance<T> &lhs, bool rhs);

template<class T>
inline bool eq2(const class_instance<T> &lhs, int rhs);

template<class T>
inline bool eq2(const class_instance<T> &lhs, double rhs);

template<class T>
inline bool eq2(const class_instance<T> &lhs, const string &rhs);

template<class T, class T1>
inline bool eq2(const class_instance<T> &lhs, const array<T1> &rhs);


inline bool eq2(bool lhs, const var &rhs);

inline bool eq2(int lhs, const var &rhs);

inline bool eq2(double lhs, const var &rhs);

inline bool eq2(const string &lhs, const var &rhs);

template<class T>
inline bool eq2(const array<T> &lhs, const var &rhs);

template<class T>
inline bool eq2(const class_instance<T> &lhs, const var &rhs);

inline bool eq2(const var &lhs, bool rhs);

inline bool eq2(const var &lhs, int rhs);

inline bool eq2(const var &lhs, double rhs);

inline bool eq2(const var &lhs, const string &rhs);

template<class T>
inline bool eq2(const var &lhs, const array<T> &rhs);

template<class T>
inline bool eq2(const var &lhs, const class_instance<T> &rhs);


template<class T1, class T2>
inline bool neq2(const T1 &lhs, const T2 &rhs);


inline bool equals(bool lhs, const var &rhs);

inline bool equals(int lhs, const var &rhs);

inline bool equals(double lhs, const var &rhs);

inline bool equals(const string &lhs, const var &rhs);

template<class T>
inline bool equals(const array<T> &lhs, const var &rhs);

template<class T>
inline bool equals(const class_instance<T> &lhs, bool rhs);

inline bool equals(const var &lhs, bool rhs);

inline bool equals(const var &lhs, int rhs);

inline bool equals(const var &lhs, double rhs);

inline bool equals(const var &lhs, const string &rhs);

template<class T>
inline bool equals(const var &lhs, const array<T> &rhs);

template<class T>
inline bool equals(const class_instance<T> &lhs, const class_instance<T> &rhs);

template<class T1, class T2>
inline bool equals(const class_instance<T1> &lhs, const class_instance<T2> &rhs);

template<class T>
inline bool equals(const var &lhs, const class_instance<T> &rhs);

template<class T>
inline bool equals(bool lhs, const class_instance<T> &rhs);

template<class T1, class T2>
inline bool not_equals(const T1 &lhs, const T2 &rhs);

template<class T>
bool eq2(const var &v, const OrFalse<T> &value);

template<class T>
bool eq2(const OrFalse<T> &value, const var &v);

template<class T>
bool equals(const OrFalse<T> &value, const var &v);

template<class T>
bool equals(const var &v, const OrFalse<T> &value);

template<class T>
bool not_equals(const OrFalse<T> &value, const var &v);

template<class T>
bool not_equals(const var &v, const OrFalse<T> &value);

