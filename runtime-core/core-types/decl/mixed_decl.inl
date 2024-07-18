// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

template<typename T>
struct is_type_acceptable_for_mixed : vk::is_type_in_list<T, long, long long, int, bool, double, string> {
};

template<typename T>
struct is_type_acceptable_for_mixed<array<T>> : is_constructible_or_unknown<mixed, T> {
};

class mixed {
public:
  enum class type {
    NUL,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    ARRAY,
  };

  mixed(const void *) = delete; // deprecate conversion from pointer to boolean
  inline mixed() = default;
  inline mixed(const Unknown &u) noexcept;
  inline mixed(const char *s, string::size_type len) noexcept;
  inline mixed(const mixed &v) noexcept;
  inline mixed(mixed &&v) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<std::decay_t<T>>::value>>
  inline mixed(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  inline mixed(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  inline mixed(Optional<T> &&v) noexcept;

  inline mixed &operator=(const mixed &other) noexcept;
  inline mixed &operator=(mixed &&other) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<std::decay_t<T>>::value>>
  inline mixed &operator=(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  inline mixed &operator=(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  inline mixed &operator=(Optional<T> &&v) noexcept;

  inline mixed &assign(const char *other, string::size_type len);

  inline const mixed operator-() const;
  inline const mixed operator+() const;

  inline int64_t operator~() const;

  inline mixed &operator+=(const mixed &other);
  inline mixed &operator-=(const mixed &other);
  inline mixed &operator*=(const mixed &other);
  inline mixed &operator/=(const mixed &other);
  inline mixed &operator%=(const mixed &other);

  inline mixed &operator&=(const mixed &other);
  inline mixed &operator|=(const mixed &other);
  inline mixed &operator^=(const mixed &other);
  inline mixed &operator<<=(const mixed &other);
  inline mixed &operator>>=(const mixed &other);

  inline mixed &operator++();
  inline const mixed operator++(int);

  inline mixed &operator--();
  inline const mixed operator--(int);

  inline bool operator!() const;

  inline mixed &append(const string &v);
  inline mixed &append(tmp_string v);

  inline mixed &operator[](int64_t int_key);
  inline mixed &operator[](int32_t key) { return (*this)[int64_t{key}]; }
  inline mixed &operator[](const string &string_key);
  inline mixed &operator[](tmp_string string_key);
  inline mixed &operator[](const mixed &v);
  inline mixed &operator[](double double_key);
  inline mixed &operator[](const array<mixed>::const_iterator &it);
  inline mixed &operator[](const array<mixed>::iterator &it);

  inline void set_value(int64_t int_key, const mixed &v);
  inline void set_value(int32_t key, const mixed &value) { set_value(int64_t{key}, value); }
  inline void set_value(const string &string_key, const mixed &v);
  inline void set_value(const string &string_key, const mixed &v, int64_t precomuted_hash);
  inline void set_value(tmp_string string_key, const mixed &v);
  inline void set_value(const mixed &v, const mixed &value);
  inline void set_value(double double_key, const mixed &value);
  inline void set_value(const array<mixed>::const_iterator &it);
  inline void set_value(const array<mixed>::iterator &it);

  inline const mixed get_value(int64_t int_key) const;
  inline const mixed get_value(int32_t key) const { return get_value(int64_t{key}); }
  inline const mixed get_value(const string &string_key) const;
  inline const mixed get_value(const string &string_key, int64_t precomuted_hash) const;
  inline const mixed get_value(tmp_string string_key) const;
  inline const mixed get_value(const mixed &v) const;
  inline const mixed get_value(double double_key) const;
  inline const mixed get_value(const array<mixed>::const_iterator &it) const;
  inline const mixed get_value(const array<mixed>::iterator &it) const;

  inline void push_back(const mixed &v);
  inline const mixed push_back_return(const mixed &v);

  inline bool isset(int64_t int_key) const;
  inline bool isset(int32_t key) const { return isset(int64_t{key}); }
  template <class ...MaybeHash>
  inline bool isset(const string &string_key, MaybeHash ...maybe_hash) const;
  inline bool isset(const mixed &v) const;
  inline bool isset(double double_key) const;

  inline void unset(int64_t int_key);
  inline void unset(int32_t key) { unset(int64_t{key}); }
  template <class ...MaybeHash>
  inline void unset(const string &string_key, MaybeHash ...maybe_hash);
  inline void unset(const mixed &v);
  inline void unset(double double_key);

  void destroy() noexcept;
  ~mixed() noexcept;

  void clear() noexcept;

  inline const mixed to_numeric() const;
  inline bool to_bool() const;
  inline int64_t to_int() const;
  inline double to_float() const;
  inline const string to_string() const;
  inline const array<mixed> to_array() const;

  inline bool &as_bool() __attribute__((always_inline));
  inline const bool &as_bool() const __attribute__((always_inline));

  inline int64_t &as_int() __attribute__((always_inline));
  inline const int64_t &as_int() const __attribute__((always_inline));

  inline double &as_double() __attribute__((always_inline));
  inline const double &as_double() const __attribute__((always_inline));

  inline string &as_string() __attribute__((always_inline));
  inline const string &as_string() const __attribute__((always_inline));

  inline array<mixed> &as_array() __attribute__((always_inline));
  inline const array<mixed> &as_array() const __attribute__((always_inline));

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
  inline const array<mixed> &as_array(const char *function) const;

  inline bool &as_bool(const char *function);
  inline int64_t &as_int(const char *function);
  inline double &as_float(const char *function);
  inline string &as_string(const char *function);
  inline array<mixed> &as_array(const char *function);

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
  inline int64_t compare(const mixed &rhs) const;

  inline array<mixed>::const_iterator begin() const;
  inline array<mixed>::const_iterator end() const;

  inline array<mixed>::iterator begin();
  inline array<mixed>::iterator end();

  inline void swap(mixed &other);

  inline int64_t get_reference_counter() const;

  inline void set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept;
  inline bool is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept;
  inline void force_destroy(ExtraRefCnt expected_ref_cnt) noexcept;

  inline size_t estimate_memory_usage() const;

  static inline void reset_empty_values() noexcept;

private:
  inline void copy_from(const mixed &other);
  inline void copy_from(mixed &&other);

  template<typename T>
  inline void init_from(T &&v);
  inline void init_from(mixed v) { copy_from(std::move(v)); }

  template<typename T>
  inline mixed &assign_from(T &&v);
  inline mixed &assign_from(mixed v) { return (*this = std::move(v)); }

  template<typename T>
  auto get_type_and_value_ptr(const array<T>  &) { return std::make_pair(type::ARRAY  , &as_array());  }
  auto get_type_and_value_ptr(const bool      &) { return std::make_pair(type::BOOLEAN, &as_bool());   }
  auto get_type_and_value_ptr(const long      &) { return std::make_pair(type::INTEGER, &as_int());    }
  auto get_type_and_value_ptr(const long long &) { return std::make_pair(type::INTEGER, &as_int());    }
  auto get_type_and_value_ptr(const int       &) { return std::make_pair(type::INTEGER, &as_int()); }
  auto get_type_and_value_ptr(const double    &) { return std::make_pair(type::FLOAT  , &as_double()); }
  auto get_type_and_value_ptr(const string    &) { return std::make_pair(type::STRING , &as_string()); }

  template<typename T>
  static T &empty_value() noexcept;

  type type_{type::NUL};
  uint64_t storage_{0};
};

