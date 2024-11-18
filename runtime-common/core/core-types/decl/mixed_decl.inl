// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/intrusive_ptr.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"


#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

template<typename T>
struct is_type_acceptable_for_mixed : vk::is_type_in_list<T, long, long long, int, bool, double, string> {
};

template<typename T>
struct is_type_acceptable_for_mixed<array<T>> : is_constructible_or_unknown<mixed, T> {
};

template<typename T>
struct is_type_acceptable_for_mixed<class_instance<T>> : std::is_base_of<may_be_mixed_base, T> {
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
    OBJECT,
  };

  mixed(const void *) = delete; // deprecate conversion from pointer to boolean
  mixed() = default;
  mixed(const Unknown &u) noexcept;
  mixed(const char *s, string::size_type len) noexcept;
  mixed(const mixed &v) noexcept;
  mixed(mixed &&v) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<std::decay_t<T>>::value>>
  mixed(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  mixed(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  mixed(Optional<T> &&v) noexcept;

  mixed &operator=(const mixed &other) noexcept;
  mixed &operator=(mixed &&other) noexcept;

  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<std::decay_t<T>>::value>>
  mixed &operator=(T &&v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  mixed &operator=(const Optional<T> &v) noexcept;
  template<typename T, typename = std::enable_if_t<is_type_acceptable_for_mixed<T>::value>>
  mixed &operator=(Optional<T> &&v) noexcept;

  mixed &assign(const char *other, string::size_type len);

  const mixed operator-() const;
  const mixed operator+() const;

  int64_t operator~() const;

  mixed &operator+=(const mixed &other);
  mixed &operator-=(const mixed &other);
  mixed &operator*=(const mixed &other);
  mixed &operator/=(const mixed &other);
  mixed &operator%=(const mixed &other);

  mixed &operator&=(const mixed &other);
  mixed &operator|=(const mixed &other);
  mixed &operator^=(const mixed &other);
  mixed &operator<<=(const mixed &other);
  mixed &operator>>=(const mixed &other);

  mixed &operator++();
  const mixed operator++(int);

  mixed &operator--();
  const mixed operator--(int);

  bool operator!() const;

  mixed &append(const string &v);
  mixed &append(tmp_string v);

  mixed &operator[](int64_t int_key);
  mixed &operator[](int32_t key) { return (*this)[int64_t{key}]; }
  mixed &operator[](const string &string_key);
  mixed &operator[](tmp_string string_key);
  mixed &operator[](const mixed &v);
  mixed &operator[](double double_key);
  mixed &operator[](const array<mixed>::const_iterator &it);
  mixed &operator[](const array<mixed>::iterator &it);

  /*
   * The `set_value_return()` method is used in assignment chains like `$mix[0] = $mix[1] = foo();`.
   * Normally, this could be transpiled to `v$mix[0] = v$mix[1] = f$foo()`. However, when `$mix` is an object
   * implementing ArrayAccess, this doesn't work because `offsetGet()` returns by value, not by reference.
   * This is why `mixed &operator[]` cannot be expressed using `offsetGet()`.
   * Since returning by reference is not supported, we call `offsetSet($offset, $value)` and return `$value`.
   */

  template<typename T>
  inline mixed set_value_return(T key, const mixed &val);
  mixed set_value_return(const mixed &key, const mixed &val);
  mixed set_value_return(const string &key, const mixed &val);
  mixed set_value_return(const array<mixed>::iterator &key, const mixed &val);
  mixed set_value_return(const array<mixed>::const_iterator &key, const mixed &val);

  void set_value(int64_t int_key, const mixed &v);
  void set_value(int32_t key, const mixed &value) { set_value(int64_t{key}, value); }
  void set_value(const string &string_key, const mixed &v);
  void set_value(const string &string_key, const mixed &v, int64_t precomuted_hash);
  void set_value(tmp_string string_key, const mixed &v);
  void set_value(const mixed &v, const mixed &value);
  void set_value(double double_key, const mixed &value);
  void set_value(const array<mixed>::const_iterator &it);
  void set_value(const array<mixed>::iterator &it);

  const mixed get_value(int64_t int_key) const;
  const mixed get_value(int32_t key) const { return get_value(int64_t{key}); }
  const mixed get_value(const string &string_key) const;
  const mixed get_value(const string &string_key, int64_t precomuted_hash) const;
  const mixed get_value(tmp_string string_key) const;
  const mixed get_value(const mixed &v) const;
  const mixed get_value(double double_key) const;
  const mixed get_value(const array<mixed>::const_iterator &it) const;
  const mixed get_value(const array<mixed>::iterator &it) const;
  template<typename ...Args>
  inline const mixed get_value_if_isset(Args &&...args) const;
  template<typename ...Args>
  inline const mixed get_value_if_not_empty(Args &&...args) const;

  void push_back(const mixed &v);
  const mixed push_back_return(const mixed &v);

  bool isset(int64_t int_key) const;
  bool isset(int32_t key) const { return isset(int64_t{key}); }
  template <class ...MaybeHash>
  bool isset(const string &string_key, MaybeHash ...maybe_hash) const;
  bool isset(const mixed &v) const;
  bool isset(double double_key) const;

  void unset(int64_t int_key);
  void unset(int32_t key) { unset(int64_t{key}); }
  template <class ...MaybeHash>
  void unset(const string &string_key, MaybeHash ...maybe_hash);
  void unset(const mixed &v);
  void unset(double double_key);

  void destroy() noexcept;
  ~mixed() noexcept;

  void clear() noexcept;

  const mixed to_numeric() const;
  bool to_bool() const;
  int64_t to_int() const;
  double to_float() const;
  const string to_string() const;
  const array<mixed> to_array() const;

  bool &as_bool() __attribute__((always_inline)) { return *reinterpret_cast<bool *>(&storage_); }
  const bool &as_bool() const __attribute__((always_inline)) { return *reinterpret_cast<const bool *>(&storage_); }

  int64_t &as_int() __attribute__((always_inline)) { return *reinterpret_cast<int64_t *>(&storage_); }
  const int64_t &as_int() const __attribute__((always_inline)) { return *reinterpret_cast<const int64_t *>(&storage_); }

  double &as_double() __attribute__((always_inline)) { return *reinterpret_cast<double *>(&storage_); }
  const double &as_double() const __attribute__((always_inline)) { return *reinterpret_cast<const double *>(&storage_); }

  string &as_string() __attribute__((always_inline)) { return *reinterpret_cast<string *>(&storage_); }
  const string &as_string() const __attribute__((always_inline)) { return *reinterpret_cast<const string *>(&storage_); }

  array<mixed> &as_array() __attribute__((always_inline)) { return *reinterpret_cast<array<mixed> *>(&storage_); }
  const array<mixed> &as_array() const __attribute__((always_inline)) { return *reinterpret_cast<const array<mixed> *>(&storage_); }

  vk::intrusive_ptr<may_be_mixed_base> as_object() __attribute__((always_inline)) { return *reinterpret_cast<vk::intrusive_ptr<may_be_mixed_base>*>(&storage_); }
  const vk::intrusive_ptr<may_be_mixed_base> as_object() const __attribute__((always_inline)) {  return *reinterpret_cast<const vk::intrusive_ptr<may_be_mixed_base>*>(&storage_);  }


  // TODO is it ok to return pointer to mutable from const method?
  // I need it just to pass such a pointer into class_instance. Mutability is needed because
  // class_instance do ref-counting
  template <typename InstanceClass, typename T = typename InstanceClass::ClassType>
  T *as_object_ptr() const {
    auto ptr_to_object = vk::dynamic_pointer_cast<T>(*reinterpret_cast<const vk::intrusive_ptr<may_be_mixed_base> *>(&storage_));
    return ptr_to_object.get();
  }

  template <typename ObjType>
  bool is_a() const {
    if (type_ != type::OBJECT) {
      return false;
    }

    auto ptr = *reinterpret_cast<const vk::intrusive_ptr<may_be_mixed_base>*>(&storage_);
    return static_cast<bool>(vk::dynamic_pointer_cast<ObjType>(ptr));
  }

  int64_t safe_to_int() const;

  void convert_to_numeric();
  void convert_to_bool();
  void convert_to_int();
  void convert_to_float();
  void convert_to_string();

  const bool &as_bool(const char *function) const;
  const int64_t &as_int(const char *function) const;
  const double &as_float(const char *function) const;
  const string &as_string(const char *function) const;
  const array<mixed> &as_array(const char *function) const;

  bool &as_bool(const char *function);
  int64_t &as_int(const char *function);
  double &as_float(const char *function);
  string &as_string(const char *function);
  array<mixed> &as_array(const char *function);

  bool is_numeric() const;
  bool is_scalar() const;

  inline type get_type() const;
  inline bool is_null() const;
  inline bool is_bool() const;
  inline bool is_int() const;
  inline bool is_float() const;
  inline bool is_string() const;
  inline bool is_array() const;
  inline bool is_object() const;

  const string get_type_str() const;
  const char *get_type_c_str() const;
  const char *get_type_or_class_name() const;


  template<typename T>
  inline bool empty_on(T key) const;
  bool empty_on(const mixed &key) const;
  bool empty_on(const string &key) const;
  bool empty_on(const string &key, int64_t precomputed_hash) const;
  bool empty_on(const array<mixed>::iterator &key) const;
  bool empty_on(const array<mixed>::const_iterator &key) const;

  bool empty() const;
  int64_t count() const;
  int64_t compare(const mixed &rhs) const;

  array<mixed>::const_iterator begin() const;
  array<mixed>::const_iterator end() const;

  array<mixed>::iterator begin();
  array<mixed>::iterator end();

  inline void swap(mixed &other);

  int64_t get_reference_counter() const;

  void set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept;
  bool is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept;
  void force_destroy(ExtraRefCnt expected_ref_cnt) noexcept;

  size_t estimate_memory_usage() const;

  static inline void reset_empty_values() noexcept;

private:
  void copy_from(const mixed &other);
  void copy_from(mixed &&other);

  template<typename T>
  inline void init_from(T &&v);
  void init_from(mixed v) { copy_from(std::move(v)); }

  template<typename T>
  inline mixed &assign_from(T &&v);
  mixed &assign_from(mixed v) { return (*this = std::move(v)); }

  template<typename T>
  auto get_type_and_value_ptr(const array<T>  &) { return std::make_pair(type::ARRAY  , &as_array());  }
  auto get_type_and_value_ptr(const bool      &) { return std::make_pair(type::BOOLEAN, &as_bool());   }
  auto get_type_and_value_ptr(const long      &) { return std::make_pair(type::INTEGER, &as_int());    }
  auto get_type_and_value_ptr(const long long &) { return std::make_pair(type::INTEGER, &as_int());    }
  auto get_type_and_value_ptr(const int       &) { return std::make_pair(type::INTEGER, &as_int()); }
  auto get_type_and_value_ptr(const double    &) { return std::make_pair(type::FLOAT  , &as_double()); }
  auto get_type_and_value_ptr(const string    &) { return std::make_pair(type::STRING , &as_string()); }
  template<typename InstanceClass>
  auto get_type_and_value_ptr(const InstanceClass   &) {return std::make_pair(type::OBJECT, as_object_ptr<InstanceClass>()); }

  template<typename T>
  static T &empty_value() noexcept;

  type type_{type::NUL};
  uint64_t storage_{0};
};

mixed operator+(const mixed &lhs, const mixed &rhs);
mixed operator-(const mixed &lhs, const mixed &rhs);
mixed operator*(const mixed &lhs, const mixed &rhs);
mixed operator-(const string &lhs);
mixed operator+(const string &lhs);
int64_t operator&(const mixed &lhs, const mixed &rhs);
int64_t operator|(const mixed &lhs, const mixed &rhs);
int64_t operator^(const mixed &lhs, const mixed &rhs);
int64_t operator<<(const mixed &lhs, const mixed &rhs);
int64_t operator>>(const mixed &lhs, const mixed &rhs);
bool operator<(const mixed &lhs, const mixed &rhs);
bool operator<=(const mixed &lhs, const mixed &rhs);
void swap(mixed &lhs, mixed &rhs);

string_buffer &operator<<(string_buffer &sb, const mixed &v);

template <typename T>
bool less_number_string_as_php8_impl(T lhs, const string &rhs);
template <typename T>
bool less_string_number_as_php8_impl(const string &lhs, T rhs);
template <typename T>
bool less_number_string_as_php8(bool php7_result, T lhs, const string &rhs);
template <typename T>
inline bool less_string_number_as_php8(bool php7_result, const string &lhs, T rhs);

template<class InputClass>
mixed f$to_mixed(const class_instance<InputClass> &instance) noexcept;
template<class ResultClass>
ResultClass from_mixed(const mixed &m, const string &) noexcept;
std::pair<class_instance<C$ArrayAccess>, bool> try_as_array_access(const mixed &) noexcept;
