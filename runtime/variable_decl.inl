#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

template<typename T>
struct is_type_acceptable_for_var : vk::is_type_in_list<T, int64_t, int32_t, bool, double, string> {
};

template<typename T>
struct is_type_acceptable_for_var<array<T>> : is_constructible_or_unknown<var, T> {
};

class var {
public:
  enum class type {
    NUL,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    ARRAY,
  };

  var(const void *) = delete; // deprecate conversion from pointer to boolean
  inline var() = default;
  inline var(const Unknown &u) noexcept;
  inline var(const char *s, string::size_type len) noexcept;
  inline var(const var &v) noexcept;
  inline var(var &&v) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<std::decay_t<T>>::value>>
  inline var(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var(Optional<T> &&v) noexcept;

  inline var &operator=(const var &other) noexcept;
  inline var &operator=(var &&other) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<std::decay_t<T>>::value>>
  inline var &operator=(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var &operator=(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_var<T>::value>>
  inline var &operator=(Optional<T> &&v) noexcept;

  inline var &assign(const char *other, string::size_type len);

  inline const var operator-() const;
  inline const var operator+() const;

  inline int64_t operator~() const;

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

  inline var &operator[](int64_t int_key);
  inline var &operator[](int32_t key) { return (*this)[int64_t{key}]; }
  inline var &operator[](const string &string_key);
  inline var &operator[](const var &v);
  inline var &operator[](double double_key);
  inline var &operator[](const array<var>::const_iterator &it);
  inline var &operator[](const array<var>::iterator &it);

  inline void set_value(int64_t int_key, const var &v);
  inline void set_value(int32_t key, const var &value) { set_value(int64_t{key}, value); }
  inline void set_value(const string &string_key, const var &v);
  inline void set_value(const string &string_key, const var &v, int64_t precomuted_hash);
  inline void set_value(const var &v, const var &value);
  inline void set_value(double double_key, const var &value);
  inline void set_value(const array<var>::const_iterator &it);
  inline void set_value(const array<var>::iterator &it);

  inline const var get_value(int64_t int_key) const;
  inline const var get_value(int32_t key) const { return get_value(int64_t{key}); }
  inline const var get_value(const string &string_key) const;
  inline const var get_value(const string &string_key, int64_t precomuted_hash) const;
  inline const var get_value(const var &v) const;
  inline const var get_value(double double_key) const;
  inline const var get_value(const array<var>::const_iterator &it) const;
  inline const var get_value(const array<var>::iterator &it) const;

  inline void push_back(const var &v);
  inline const var push_back_return(const var &v);

  inline bool isset(int64_t int_key) const;
  inline bool isset(int32_t key) const { return isset(int64_t{key}); }
  inline bool isset(const string &string_key) const;
  inline bool isset(const var &v) const;
  inline bool isset(double double_key) const;

  inline void unset(int64_t int_key);
  inline void unset(int32_t key) { unset(int64_t{key}); }
  inline void unset(const string &string_key);
  inline void unset(const var &v);
  inline void unset(double double_key);

  inline void destroy();
  inline ~var();

  inline void clear();

  inline const var to_numeric() const;
  inline bool to_bool() const;
  inline int64_t to_int() const;
  inline double to_float() const;
  inline const string to_string() const;
  inline const array<var> to_array() const;

  inline bool &as_bool() __attribute__((always_inline));
  inline const bool &as_bool() const __attribute__((always_inline));

  inline int64_t &as_int() __attribute__((always_inline));
  inline const int64_t &as_int() const __attribute__((always_inline));

  inline double &as_double() __attribute__((always_inline));
  inline const double &as_double() const __attribute__((always_inline));

  inline string &as_string() __attribute__((always_inline));
  inline const string &as_string() const __attribute__((always_inline));

  inline array<var> &as_array() __attribute__((always_inline));
  inline const array<var> &as_array() const __attribute__((always_inline));

  inline int64_t safe_to_int() const;

  inline void convert_to_numeric();
  inline void convert_to_bool();
  inline void convert_to_int();
  inline void convert_to_float();
  inline void convert_to_string();

  inline const bool &as_bool(const char *function) const;
  inline const int64_t &as_int(const char *function) const;
  inline const double &as_float(const char *function) const;
  inline const string &as_string(const char *function) const;
  inline const array<var> &as_array(const char *function) const;

  inline bool &as_bool(const char *function);
  inline int64_t &as_int(const char *function);
  inline double &as_float(const char *function);
  inline string &as_string(const char *function);
  inline array<var> &as_array(const char *function);

  inline bool is_numeric() const;
  inline bool is_scalar() const;

  inline type get_type() const;
  inline bool is_null() const;
  inline bool is_bool() const;
  inline bool is_int() const;
  inline bool is_float() const;
  inline bool is_string() const;
  inline bool is_array() const;

  inline const string get_type_str() const;
  inline const char *get_type_c_str() const;

  inline bool empty() const;
  inline int64_t count() const;
  inline int64_t compare(const var &rhs) const;

  inline array<var>::const_iterator begin() const;
  inline array<var>::const_iterator end() const;

  inline array<var>::iterator begin();
  inline array<var>::iterator end();

  inline void swap(var &other);

  inline int64_t get_reference_counter() const;

  inline void set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept;
  inline bool is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept;
  inline void force_destroy(ExtraRefCnt expected_ref_cnt) noexcept;

  inline size_t estimate_memory_usage() const;

  static inline void reset_empty_values() noexcept;

private:
  inline void copy_from(const var &other);
  inline void copy_from(var &&other);

  template<typename T>
  inline void init_from(T &&v);
  inline void init_from(var v) { copy_from(std::move(v)); }

  template<typename T>
  inline var &assign_from(T &&v);
  inline var &assign_from(var v) { return (*this = std::move(v)); }

  template<typename T>
  auto get_type_and_value_ptr(const array<T> &) { return std::make_pair(type::ARRAY  , &as_array());  }
  auto get_type_and_value_ptr(const bool     &) { return std::make_pair(type::BOOLEAN, &as_bool());   }
  auto get_type_and_value_ptr(const int64_t  &) { return std::make_pair(type::INTEGER, &as_int());    }
  auto get_type_and_value_ptr(const int      &) { return std::make_pair(type::INTEGER, &as_int()); }
  auto get_type_and_value_ptr(const double   &) { return std::make_pair(type::FLOAT  , &as_double()); }
  auto get_type_and_value_ptr(const string   &) { return std::make_pair(type::STRING , &as_string()); }

  template<typename T>
  static T &empty_value() noexcept;

  type type_{type::NUL};
  uint64_t storage_{0};
};

