#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<typename T>
struct is_type_acceptable_for_var : vk::is_type_in_list<T, bool, int, double, string> {
};

template<typename T>
struct is_type_acceptable_for_var<array<T>> : is_constructible_or_unknown<var, T> {
};

class var {
  enum var_type {
    NULL_TYPE,
    BOOLEAN_TYPE,
    INTEGER_TYPE,
    FLOAT_TYPE,
    STRING_TYPE,
    ARRAY_TYPE
  };

  var_type type{NULL_TYPE};
  uint64_t storage{0};

  inline void copy_from(const var &other);
  inline void copy_from(var &&other);

  template<typename T>
  inline void init_from(T &&v);

  template<typename T>
  inline var &assign_from(T &&v);

  template<typename T>
  auto get_type_and_value_ptr(const array<T> &) { return std::make_pair(ARRAY_TYPE  , &as_array());  }
  auto get_type_and_value_ptr(const bool     &) { return std::make_pair(BOOLEAN_TYPE, &as_bool());   }
  auto get_type_and_value_ptr(const int      &) { return std::make_pair(INTEGER_TYPE, &as_int());    }
  auto get_type_and_value_ptr(const double   &) { return std::make_pair(FLOAT_TYPE  , &as_double()); }
  auto get_type_and_value_ptr(const string   &) { return std::make_pair(STRING_TYPE , &as_string()); }

public:
  var(const void *) = delete; // deprecate conversion from pointer to boolean
  inline var() = default;
  inline var(const Unknown &u);
  inline var(const char *s, int len);
  inline var(const var &v);
  inline var(var &&v);

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<std::decay_t<T>>::value>>
  inline var(T &&v);
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var(const Optional<T> &v);
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var(Optional<T> &&v);

  inline var &operator=(const var &other);
  inline var &operator=(var &&other);

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<std::decay_t<T>>::value>>
  inline var &operator=(T &&v);
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var &operator=(const Optional<T> &v);
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var &operator=(Optional<T> &&v);

  inline var &assign(const char *other, int len);

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

  inline var &operator++();
  inline const var operator++(int);

  inline var &operator--();
  inline const var operator--(int);

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

  inline void unset(int int_key);
  inline void unset(const string &string_key);
  inline void unset(const var &v);

  inline void destroy();
  inline ~var();

  inline void clear();

  inline const var to_numeric() const;
  inline bool to_bool() const;
  inline int to_int() const;
  inline double to_float() const;
  inline const string to_string() const;
  inline const array<var> to_array() const;

  inline bool &as_bool() __attribute__((always_inline));
  inline const bool &as_bool() const __attribute__((always_inline));

  inline int &as_int() __attribute__((always_inline));
  inline const int &as_int() const __attribute__((always_inline));

  inline double &as_double() __attribute__((always_inline));
  inline const double &as_double() const __attribute__((always_inline));

  inline string &as_string() __attribute__((always_inline));
  inline const string &as_string() const __attribute__((always_inline));

  inline array<var> &as_array() __attribute__((always_inline));
  inline const array<var> &as_array() const __attribute__((always_inline));

  inline int safe_to_int() const;

  inline void convert_to_numeric();
  inline void convert_to_bool();
  inline void convert_to_int();
  inline void convert_to_float();
  inline void convert_to_string();

  inline void safe_convert_to_int();

  inline const bool &as_bool(const char *function) const;
  inline const int &as_int(const char *function) const;
  inline const double &as_float(const char *function) const;
  inline const string &as_string(const char *function) const;
  inline const array<var> &as_array(const char *function) const;

  inline bool &as_bool(const char *function);
  inline int &as_int(const char *function);
  inline double &as_float(const char *function);
  inline string &as_string(const char *function);
  inline array<var> &as_array(const char *function);

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

  inline dl::size_type estimate_memory_usage() const;

  inline friend const var operator-(const string &lhs);

  inline friend const var operator+(const var &lhs, const var &rhs);
  inline friend const var operator-(const var &lhs, const var &rhs);
  inline friend const var operator*(const var &lhs, const var &rhs);
  inline friend const var operator/(const var &lhs, const var &rhs);
  inline friend const var operator%(const var &lhs, const var &rhs);

  inline friend bool eq2(const var &lhs, const var &rhs);
  inline friend bool neq2(const var &lhs, const var &rhs);
  inline friend bool operator<=(const var &lhs, const var &rhs);
  inline friend bool operator>=(const var &lhs, const var &rhs);
  inline friend bool operator<(const var &lhs, const var &rhs);
  inline friend bool operator>(const var &lhs, const var &rhs);
  inline friend bool equals(const var &lhs, const var &rhs);

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

inline bool eq2(const var &lhs, const var &rhs);
inline bool neq2(const var &lhs, const var &rhs);
inline bool operator<=(const var &lhs, const var &rhs);
inline bool operator>=(const var &lhs, const var &rhs);
inline bool operator<(const var &lhs, const var &rhs);
inline bool operator>(const var &lhs, const var &rhs);
inline bool equals(const var &lhs, const var &rhs);


inline void swap(var &lhs, var &rhs);


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
inline bool eq2(bool lhs, const std::tuple<Args...> &rhs);

template<class T>
inline bool eq2(int lhs, const array<T> &rhs);

template<class T>
inline bool eq2(double lhs, const array<T> &rhs);

template<class T>
inline bool eq2(const string &lhs, const array<T> &rhs);

template<class T>
inline bool eq2(const array<T> &lhs, bool rhs);

template<class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, bool rhs);

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
std::enable_if_t<std::is_base_of<T1, T2>{} || std::is_base_of<T2, T1>{}, bool> equals(const class_instance<T1> &lhs, const class_instance<T2> &rhs);

template<class T1, class T2>
std::enable_if_t<!std::is_base_of<T1, T2>{} && !std::is_base_of<T2, T1>{}, bool>  equals(const class_instance<T1> &, const class_instance<T2> &);

template<class T>
inline bool equals(const var &lhs, const class_instance<T> &rhs);

template<class T>
inline bool equals(bool lhs, const class_instance<T> &rhs);


