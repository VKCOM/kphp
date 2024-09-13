// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/find.h"
#include "common/smart_ptrs/intrusive_ptr.h"

#include "runtime-core/utils/migration-php8.h"
#include "runtime-core/class-instance/refcountable-php-classes.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

static_assert(vk::all_of_equal(sizeof(string), sizeof(double), sizeof(array<mixed>)), "sizeof of array<mixed>, string and double must be equal");

template<typename T>
void mixed::init_from(T &&v) {
  if constexpr (is_class_instance_v<std::decay_t<T>>) {
    static_assert(sizeof(storage_) >= sizeof(vk::intrusive_ptr<may_be_mixed_base>));
    auto ptr_to_obj = new(&storage_) vk::intrusive_ptr<may_be_mixed_base>(dynamic_cast<may_be_mixed_base*>(v.get()));
    if (unlikely(!ptr_to_obj)) {
      php_error("Internal error. Trying to set invalid object to mixed");
    }
    type_ = type::OBJECT;
  } else {
    auto type_and_value_ref = get_type_and_value_ptr(v);
    type_ = type_and_value_ref.first;
    auto *value_ptr = type_and_value_ref.second;
    using ValueType = std::decay_t<decltype(*value_ptr)>;
    new(value_ptr) ValueType(std::forward<T>(v));
  }
}

template<typename T>
mixed &mixed::assign_from(T &&v) {
  if constexpr(is_class_instance_v<std::decay_t<T>>) {
    static_assert(std::is_base_of_v<may_be_mixed_base, typename std::decay_t<T>::ClassType>);
    destroy();
    init_from(std::forward<T>(v));
  } else {
    auto type_and_value_ref = get_type_and_value_ptr(v);
    if (get_type() == type_and_value_ref.first) {
      *type_and_value_ref.second = std::forward<T>(v);
    } else {
      destroy();
      init_from(std::forward<T>(v));
    }
  }

  return *this;
}

template<typename T, typename>
mixed::mixed(T &&v) noexcept {
  init_from(std::forward<T>(v));
}

template<typename T, typename>
mixed::mixed(const Optional<T> &v) noexcept {
  auto init_from_lambda = [this](const auto &v) { this->init_from(v); };
  call_fun_on_optional_value(init_from_lambda, v);
}

template<typename T, typename>
mixed::mixed(Optional<T> &&v) noexcept {
   auto init_from_lambda = [this](auto &&v) { this->init_from(std::move(v)); };
   call_fun_on_optional_value(init_from_lambda, std::move(v));
}

template<typename T, typename>
mixed &mixed::operator=(T &&v) noexcept {
  return assign_from(std::forward<T>(v));
}

template<typename T, typename>
mixed &mixed::operator=(const Optional<T> &v) noexcept {
  auto assign_from_lambda = [this](const auto &v) -> mixed& { return this->assign_from(v); };
  return call_fun_on_optional_value(assign_from_lambda, v);
}

template<typename T, typename>
mixed &mixed::operator=(Optional<T> &&v) noexcept {
  auto assign_from_lambda = [this](auto &&v) -> mixed& { return this->assign_from(std::move(v)); };
  return call_fun_on_optional_value(assign_from_lambda, std::move(v));
}

template<class T1, class T2>
int64_t spaceship(const T1 &lhs, const T2 &rhs);

template <class ...MaybeHash>
bool mixed::isset(const string &string_key, MaybeHash ...maybe_hash) const {
  if (unlikely (get_type() != type::ARRAY)) {
    int64_t int_key{std::numeric_limits<int64_t>::max()};
    if (get_type() == type::STRING) {
      if (!string_key.try_to_int(&int_key)) {
        php_warning("\"%s\" is illegal offset for string", string_key.c_str());
        return false;
      }
    }

    return isset(int_key);
  }

  return as_array().isset(string_key, maybe_hash...);
}

template <class ...MaybeHash>
void mixed::unset(const string &string_key, MaybeHash ...maybe_hash) {
  if (unlikely (get_type() != type::ARRAY)) {
    if (get_type() != type::NUL && (get_type() != type::BOOLEAN || as_bool())) {
      php_warning("Cannot use variable of type %s as array in unset", get_type_or_class_name());
    }
    return;
  }

  as_array().unset(string_key, maybe_hash...);
}


inline mixed::type mixed::get_type() const {
  return type_;
}

inline bool mixed::is_null() const {
  return get_type() == type::NUL;
}

inline bool mixed::is_bool() const {
  return get_type() == type::BOOLEAN;
}

inline bool mixed::is_int() const {
  return get_type() == type::INTEGER;
}

inline bool mixed::is_float() const {
  return get_type() == type::FLOAT;
}

inline bool mixed::is_string() const {
  return get_type() == type::STRING;
}

inline bool mixed::is_array() const {
  return get_type() == type::ARRAY;
}

inline bool mixed::is_object() const {
  return get_type() == type::OBJECT;
}

inline void mixed::swap(mixed &other) {
  ::swap(type_, other.type_);
  ::swap(as_double(), other.as_double());
}

inline void swap(mixed &lhs, mixed &rhs) {
  lhs.swap(rhs);
}

template<typename T>
T &mixed::empty_value() noexcept {
  static_assert(vk::is_type_in_list<T, bool, int64_t, double, string, mixed, array<mixed>>{} || is_type_acceptable_for_mixed<T>::value, "unsupported type");

  static T value;
  value = T{};
  return value;
}

inline void mixed::reset_empty_values() noexcept {
  empty_value<bool>();
  empty_value<int64_t>();
  empty_value<double>();
  empty_value<string>();
  empty_value<mixed>();
  empty_value<array<mixed>>();
}

template<class T>
string_buffer &operator<<(string_buffer &sb, const Optional<T> &v) {
  auto write_lambda = [&sb](const auto &v) -> string_buffer& { return sb << v; };
  return call_fun_on_optional_value(write_lambda, v);
}


template <typename Arg1, typename Arg2>
const char *conversion_php_warning_string() {
  return "";
}

template<>
inline const char *conversion_php_warning_string<int64_t, string>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %" PRIi64 " and \"%s\" (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<double, string>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for %lf and \"%s\" (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<string, int64_t>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %" PRIi64 " (PHP7: %s, PHP8: %s)";
}

template<>
inline const char *conversion_php_warning_string<string, double>() {
  return "Comparison (operator <) results in PHP 7 and PHP 8 are different for \"%s\" and %lf (PHP7: %s, PHP8: %s)";
}

template <typename T>
bool less_number_string_as_php8_impl(T lhs, const string &rhs) {
  auto rhs_float = 0.0;
  const auto rhs_is_string_number = rhs.try_to_float(&rhs_float);

  if (rhs_is_string_number) {
    return lhs < rhs_float;
  } else {
    return compare_strings_php_order(string(lhs), rhs) < 0;
  }
}

template <typename T>
bool less_string_number_as_php8_impl(const string &lhs, T rhs) {
  auto lhs_float = 0.0;
  const auto lhs_is_string_number = lhs.try_to_float(&lhs_float);

  if (lhs_is_string_number) {
    return lhs_float < rhs;
  } else {
    return compare_strings_php_order(lhs, string(rhs)) < 0;
  }
}

template <typename T>
bool less_number_string_as_php8(bool php7_result, T lhs, const string &rhs) {
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = less_number_string_as_php8_impl(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning(conversion_php_warning_string<typename std::decay<decltype(lhs)>::type, typename std::decay<decltype(rhs)>::type>(),
                lhs,
                rhs.c_str(),
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}

template <typename T>
bool less_string_number_as_php8(bool php7_result, const string &lhs, T rhs) {
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = less_string_number_as_php8_impl(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning(conversion_php_warning_string<typename std::decay<decltype(lhs)>::type, typename std::decay<decltype(rhs)>::type>(),
                lhs.c_str(),
                rhs,
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}

template<class InputClass>
mixed f$to_mixed(const class_instance<InputClass> &instance) noexcept {
  mixed m;
  m = instance;
  return m;
}

template<class ResultClass>
ResultClass from_mixed(const mixed &m, const string &) noexcept {
  if constexpr (!std::is_polymorphic_v<typename ResultClass::ClassType>) {
    php_error("Internal error. Class inside a mixed is not polymorphic");
    return {};
  } else {
    return ResultClass::create_from_base_raw_ptr(dynamic_cast<abstract_refcountable_php_interface *>(m.as_object_ptr<ResultClass>()));
  }
}