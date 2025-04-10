#pragma once

#include "runtime-common/core/utils/migration-php8.h"

#ifndef INCLUDED_FROM_KPHP_CORE
#error "this file must be included only from runtime-core.h"
#endif

namespace impl_ {

template<class T1, class T2>
inline bool optional_eq2_impl(const Optional<T1>& lhs, const T2& rhs);
template<class T1, class T2>
inline bool optional_equals_impl(const Optional<T1>& lhs, const T2& rhs);

template<class T1, class T2>
inline bool optional_lt_impl(const Optional<T1>& lhs, const T2& rhs);
template<class T1, class T2>
inline enable_if_t_is_not_optional<T1, bool> optional_lt_impl(const T1& lhs, const Optional<T2>& rhs);

} // namespace impl_

template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT&& fun, const Optional<T>& opt, Args&&... args);
template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT&& fun, Optional<T>&& opt, Args&&... args);

template<class T, class U>
inline enable_if_one_of_types_is_unknown<T, U> eq2(const T&, const U&) {
  php_assert("Comparison of Unknown" && 0);
  return false;
}

inline bool eq2(bool lhs, bool rhs) {
  return lhs == rhs;
}
inline bool eq2(int64_t lhs, int64_t rhs) {
  return lhs == rhs;
}
inline bool eq2(double lhs, double rhs) {
  return lhs == rhs;
}
inline bool eq2(const string& lhs, const string& rhs) {
  return compare_strings_php_order(lhs, rhs) == 0;
}

inline bool eq2(const mixed& lhs, const mixed& rhs) {
  if (lhs.is_object() || rhs.is_object()) {
    php_warning("operators ==, != are not supported for %s and %s", lhs.get_type_or_class_name(), rhs.get_type_or_class_name());
    return false;
  }
  return lhs.compare(rhs) == 0;
}

inline bool eq2(bool lhs, int64_t rhs) {
  return lhs != !rhs;
}
inline bool eq2(int64_t lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(bool lhs, double rhs) {
  return lhs != (rhs == 0.0);
}
inline bool eq2(double lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(int64_t lhs, double rhs) {
  return static_cast<double>(lhs) == rhs;
}
inline bool eq2(double lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(bool lhs, const string& rhs) {
  return lhs == rhs.to_bool();
}
inline bool eq2(const string& lhs, bool rhs) {
  return eq2(rhs, lhs);
}

// see https://www.php.net/manual/en/migration80.incompatible.php#migration80.incompatible.core.string-number-comparision
template<typename T>
inline bool eq2_number_string_as_php8(T lhs, const string& rhs) {
  auto rhs_float = 0.0;
  const auto rhs_is_string_number = rhs.try_to_float(&rhs_float);

  if (rhs_is_string_number) {
    return eq2(lhs, rhs_float);
  } else {
    return eq2(string(lhs), rhs);
  }
}

inline bool eq2(int64_t lhs, const string& rhs) {
  const auto php7_result = eq2(lhs, rhs.to_float());
  if (RuntimeContext::get().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = eq2_number_string_as_php8(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning("Comparison (operator ==) results in PHP 7 and PHP 8 are different for %" PRIi64 " and \"%s\" (PHP7: %s, PHP8: %s)", lhs, rhs.c_str(),
                php7_result ? "true" : "false", php8_result ? "true" : "false");
  }

  return php7_result;
}
inline bool eq2(const string& lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(double lhs, const string& rhs) {
  const auto php7_result = lhs == rhs.to_float();
  if (RuntimeContext::get().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = eq2_number_string_as_php8(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning("Comparison (operator ==) results in PHP 7 and PHP 8 are different for %lf and \"%s\" (PHP7: %s, PHP8: %s)", lhs, rhs.c_str(),
                php7_result ? "true" : "false", php8_result ? "true" : "false");
  }

  return php7_result;
}
inline bool eq2(const string& lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(bool lhs, const array<T>& rhs) {
  return lhs == !rhs.empty();
}
template<class T>
inline bool eq2(const array<T>& lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(int64_t, const array<T>&) {
  php_warning("Unsupported operand types for operator == (int and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T>& lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(double, const array<T>&) {
  php_warning("Unsupported operand types for operator == (float and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T>& lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(const string&, const array<T>&) {
  php_warning("Unsupported operand types for operator == (string and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T>& lhs, const string& rhs) {
  return eq2(rhs, lhs);
}

template<class TupleLhsT, class TupleRhsT, size_t... Indices>
inline bool eq2(const TupleLhsT& lhs, const TupleRhsT& rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, eq2(std::get<Indices>(lhs), std::get<Indices>(rhs))...);
}

template<class... Args>
inline bool eq2(const std::tuple<Args...>& lhs, const std::tuple<Args...>& rhs) {
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class... Args, class... Args2>
inline std::enable_if_t<sizeof...(Args) == sizeof...(Args2), bool> eq2(const std::tuple<Args...>& lhs, const std::tuple<Args2...>& rhs) {
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class... Args, class... Args2>
inline std::enable_if_t<sizeof...(Args) != sizeof...(Args2), bool> eq2(const std::tuple<Args...>&, const std::tuple<Args2...>&) {
  return false;
}

template<class TupleT, class Arg, size_t... Indices>
inline bool eq2(const TupleT& lhs, const array<Arg>& rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, eq2(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<class Arg, class... Args>
inline bool eq2(const std::tuple<Args...>& lhs, const array<Arg>& rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().size) {
    return false;
  }
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<class Arg, class... Args>
inline bool eq2(const array<Arg>& lhs, const std::tuple<Args...>& rhs) {
  return eq2(rhs, lhs);
}

template<class... Args>
inline bool eq2(bool lhs, const std::tuple<Args...>&) {
  return lhs;
}
template<class... Args>
inline bool eq2(const std::tuple<Args...>& lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<class T, class... Args>
inline bool eq2(const T&, const std::tuple<Args...>&) {
  return false;
}
template<class T, class... Args>
inline bool eq2(const std::tuple<Args...>& lhs, const T& rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(const class_instance<T>& lhs, const class_instance<T>& rhs) {
  // can be called implicitly from in_array, for example
  return lhs == rhs;
}

inline bool eq2(bool lhs, const mixed& rhs) {
  return lhs == rhs.to_bool();
}
inline bool eq2(const mixed& lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(double lhs, const mixed& rhs) {
  switch (rhs.get_type()) {
  case mixed::type::NUL:
    return eq2(lhs, 0.0);
  case mixed::type::BOOLEAN:
    return eq2(lhs, rhs.as_bool());
  case mixed::type::INTEGER:
    return eq2(lhs, rhs.as_int());
  case mixed::type::FLOAT:
    return eq2(lhs, rhs.as_double());
  case mixed::type::STRING:
    return eq2(lhs, rhs.as_string());
  case mixed::type::ARRAY:
    php_warning("Unsupported operand types for operator == (float and array)");
    return false;
  case mixed::type::OBJECT:
    php_warning("Unsupported operand types for operator == (float and %s)", rhs.as_object()->get_class());
    return false;
  default:
    __builtin_unreachable();
  }
}
inline bool eq2(const mixed& lhs, double rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(int64_t lhs, const mixed& rhs) {
  switch (rhs.get_type()) {
  case mixed::type::NUL:
    return eq2(lhs, int64_t{0});
  case mixed::type::BOOLEAN:
    return eq2(lhs, rhs.as_bool());
  case mixed::type::INTEGER:
    return eq2(lhs, rhs.as_int());
  case mixed::type::FLOAT:
    return eq2(lhs, rhs.as_double());
  case mixed::type::STRING:
    return eq2(lhs, rhs.as_string());
  case mixed::type::ARRAY:
    php_warning("Unsupported operand types for operator == (int and array)");
    return false;
  case mixed::type::OBJECT:
    php_warning("Unsupported operand types for operator == (int and %s)", rhs.as_object()->get_class());
    return false;
  default:
    __builtin_unreachable();
  }
}

inline bool eq2(const mixed& lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(const string& lhs, const mixed& rhs) {
  return eq2(mixed(lhs), rhs);
}
inline bool eq2(const mixed& lhs, const string& rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(const array<T>& lhs, const mixed& rhs) {
  if (likely(rhs.is_array())) {
    return eq2(lhs, rhs.as_array());
  }

  if (rhs.is_bool()) {
    return lhs.empty() != rhs.as_bool();
  }
  if (rhs.is_null()) {
    return lhs.empty();
  }

  php_warning("Unsupported operand types for operator == (array and %s)", rhs.get_type_c_str());
  return false;
}
template<class T>
inline bool eq2(const mixed& lhs, const array<T>& rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1>& lhs, const T2& rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool eq2(const T1& lhs, const Optional<T2>& rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1>& lhs, const Optional<T2>& rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}

template<class T>
inline bool eq2(const Optional<T>& lhs, const Optional<T>& rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}

template<class T1, class T2>
inline bool eq2(const array<T1>& lhs, const array<T2>& rhs) {
  if (rhs.count() != lhs.count()) {
    return false;
  }

  for (auto rhs_it = rhs.begin(); rhs_it != rhs.end(); ++rhs_it) {
    auto key = rhs_it.get_key();
    if (!lhs.has_key(key) || !eq2(lhs.get_value(key), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}

template<class T>
inline bool eq2(const T& lhs, int32_t rhs);

template<class T>
inline bool eq2(int32_t lhs, const T& rhs);

template<class T>
inline bool eq2(const T& lhs, int32_t rhs) {
  return eq2(lhs, int64_t{rhs});
}

template<class T>
inline bool eq2(int32_t lhs, const T& rhs) {
  return eq2(int64_t{lhs}, rhs);
}

template<class T, class U>
inline enable_if_one_of_types_is_unknown<T, U> equals(const T& lhs, const U& rhs) {
  return eq2(lhs, rhs);
}
template<class T, class U>
inline disable_if_one_of_types_is_unknown<T, U> equals(const T&, const U&) {
  return false;
}
template<class T>
inline disable_if_one_of_types_is_unknown<T, T> equals(const T& lhs, const T& rhs) {
  return lhs == rhs;
}

template<class T>
inline bool equals(const array<T>& lhs, const array<T>& rhs) {
  if (lhs.count() != rhs.count()) {
    return false;
  }

  for (auto lhs_it = lhs.begin(), rhs_it = rhs.begin(); lhs_it != lhs.end(); ++lhs_it, ++rhs_it) {
    if (!equals(lhs_it.get_key(), rhs_it.get_key()) || !equals(lhs_it.get_value(), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}
template<class T1, class T2>
inline bool equals(const array<T1>& lhs, const array<T2>& rhs) {
  if (lhs.count() != rhs.count()) {
    return false;
  }

  auto rhs_it = rhs.begin();
  for (auto lhs_it = lhs.begin(); lhs_it != lhs.end(); ++lhs_it, ++rhs_it) {
    if (!equals(lhs_it.get_key(), rhs_it.get_key()) || !equals(lhs_it.get_value(), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}

inline bool equals(const may_be_mixed_base* lhs, const may_be_mixed_base* rhs) {
  return lhs == rhs;
}

inline bool equals(bool lhs, const mixed& rhs) {
  return rhs.is_bool() && equals(lhs, rhs.as_bool());
}
inline bool equals(const mixed& lhs, bool rhs) {
  return equals(rhs, lhs);
}

inline bool equals(int64_t lhs, const mixed& rhs) {
  return rhs.is_int() && equals(lhs, rhs.as_int());
}
inline bool equals(const mixed& lhs, int64_t rhs) {
  return equals(rhs, lhs);
}

inline bool equals(double lhs, const mixed& rhs) {
  return rhs.is_float() && equals(lhs, rhs.as_double());
}
inline bool equals(const mixed& lhs, double rhs) {
  return equals(rhs, lhs);
}

inline bool equals(const string& lhs, const mixed& rhs) {
  return rhs.is_string() && equals(lhs, rhs.as_string());
}
inline bool equals(const mixed& lhs, const string& rhs) {
  return equals(rhs, lhs);
}

template<class T>
inline bool equals(const array<T>& lhs, const mixed& rhs) {
  return rhs.is_array() && equals(lhs, rhs.as_array());
}
template<class T>
inline bool equals(const mixed& lhs, const array<T>& rhs) {
  return equals(rhs, lhs);
}

inline bool equals(const mixed& lhs, const mixed& rhs) {
  if (lhs.get_type() != rhs.get_type()) {
    return false;
  }

  switch (lhs.get_type()) {
  case mixed::type::NUL:
    return true;
  case mixed::type::BOOLEAN:
    return equals(lhs.as_bool(), rhs.as_bool());
  case mixed::type::INTEGER:
    return equals(lhs.as_int(), rhs.as_int());
  case mixed::type::FLOAT:
    return equals(lhs.as_double(), rhs.as_double());
  case mixed::type::STRING:
    return equals(lhs.as_string(), rhs.as_string());
  case mixed::type::ARRAY:
    return equals(lhs.as_array(), rhs.as_array());
  case mixed::type::OBJECT:
    return equals(lhs.as_object(), rhs.as_object());
  default:
    __builtin_unreachable();
  }
}

template<class T>
inline bool equals(const class_instance<T>& lhs, const class_instance<T>& rhs) {
  return lhs == rhs;
}

template<class TupleLhsT, class TupleRhsT, size_t... Indices>
inline bool equals(const TupleLhsT& lhs, const TupleRhsT& rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, equals(std::get<Indices>(lhs), std::get<Indices>(rhs))...);
}

template<class... Args>
inline bool equals(const std::tuple<Args...>& lhs, const std::tuple<Args...>& rhs) {
  return equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class... Args, class... Args2>
inline bool equals(const std::tuple<Args...>& lhs, const std::tuple<Args2...>& rhs) {
  return sizeof...(Args) == sizeof...(Args2) && equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class TupleT, class Arg, size_t... Indices>
inline bool equals(const TupleT& lhs, const array<Arg>& rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, equals(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<class Arg, class... Args>
inline bool equals(const std::tuple<Args...>& lhs, const array<Arg>& rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().size) {
    return false;
  }
  return equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<class Arg, class... Args>
inline bool equals(const array<Arg>& lhs, const std::tuple<Args...>& rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2>
inline std::enable_if_t<std::is_base_of<T1, T2>{} || std::is_base_of<T2, T1>{}, bool> equals(const class_instance<T1>& lhs, const class_instance<T2>& rhs) {
  return dynamic_cast<void*>(lhs.get()) == dynamic_cast<void*>(rhs.get());
}
template<class T1, class T2>
inline std::enable_if_t<!std::is_base_of<T1, T2>{} && !std::is_base_of<T2, T1>{}, bool> equals(const class_instance<T1>&, const class_instance<T2>&) {
  return false;
}

template<class T1, class T2>
inline bool equals(const Optional<T1>& lhs, const T2& rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool equals(const T1& lhs, const Optional<T2>& rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2>
inline bool equals(const Optional<T1>& lhs, const Optional<T2>& rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}

template<class T>
inline bool equals(const Optional<T>& lhs, const Optional<T>& rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}

template<class T>
inline bool equals(const T& lhs, int32_t rhs);

template<class T>
inline bool equals(int32_t lhs, const T& rhs);

template<class T>
inline bool equals(const T& lhs, int32_t rhs) {
  return equals(lhs, int64_t{rhs});
}

template<class T>
inline bool equals(int32_t lhs, const T& rhs) {
  return equals(int64_t{lhs}, rhs);
}

inline bool lt(const bool& lhs, const bool& rhs) {
  return lhs < rhs;
}
template<class T1, class T2>
inline bool lt(const T1& lhs, const T2& rhs) {
  return lhs < rhs;
}

template<class T2>
inline bool lt(const bool& lhs, const T2& rhs) {
  return lhs < f$boolval(rhs);
}
template<class T1>
inline bool lt(const T1& lhs, const bool& rhs) {
  return f$boolval(lhs) < rhs;
}

template<class T1, class T2>
inline bool lt(const Optional<T1>& lhs, const T2& rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool lt(const T1& lhs, const Optional<T2>& rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}

template<class T>
inline bool lt(const Optional<T>& lhs, const Optional<T>& rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool lt(const Optional<T1>& lhs, const Optional<T2>& rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}

template<class T>
inline bool lt(const bool& lhs, const Optional<T>& rhs) {
  return lt(lhs, f$boolval(rhs));
}
template<class T>
inline bool lt(const Optional<T>& lhs, const bool& rhs) {
  return lt(f$boolval(lhs), rhs);
}

template<class T1, class T2>
inline bool lt(const array<T1>& lhs, const array<T2>& rhs) {
  if (lhs.count() != rhs.count()) {
    return lhs.count() < rhs.count();
  }

  for (auto lhs_it = lhs.begin(); lhs_it != lhs.end(); ++lhs_it) {
    auto key = lhs_it.get_key();
    if (!rhs.has_key(key)) {
      return false;
    }
    auto lhs_val = lhs_it.get_value();
    auto rhs_val = rhs.get_value(key);
    if (lt(lhs_val, rhs_val)) {
      return true;
    } else if (lt(rhs_val, lhs_val)) {
      return false;
    }
  }

  return false;
}

template<class T1, class T2>
inline bool leq(const T1& lhs, const T2& rhs) {
  // (lhs <= rhs) === (!(rhs < lhs))
  return !lt(rhs, lhs);
}

namespace impl_ {

template<class T1, class T2>
bool optional_eq2_impl(const Optional<T1>& lhs, const T2& rhs) {
  auto eq2_lambda = [](const auto& l, const auto& r) { return eq2(r, l); };
  return call_fun_on_optional_value(eq2_lambda, lhs, rhs);
}

template<class T1, class T2>
bool optional_equals_impl(const Optional<T1>& lhs, const T2& rhs) {
  auto equals_lambda = [](const auto& l, const auto& r) {
    // if (is_null(lhs)) { return is_null(r); }
    // else return equals(r, l); - parameters are swapped to cope with Optional on right side
    return std::is_same<decltype(l), const mixed&>{} ? f$is_null(r) : equals(r, l);
  };
  return call_fun_on_optional_value(equals_lambda, lhs, rhs);
}

template<class T1, class T2>
inline bool optional_lt_impl(const Optional<T1>& lhs, const T2& rhs) {
  auto lt_lambda = [](const auto& l, const auto& r) { return lt(l, r); };
  return call_fun_on_optional_value(lt_lambda, lhs, rhs);
}

template<class T1, class T2>
inline enable_if_t_is_not_optional<T1, bool> optional_lt_impl(const T1& lhs, const Optional<T2>& rhs) {
  auto lt_reversed_args_lambda = [](const auto& l, const auto& r) { return lt(r, l); };
  return call_fun_on_optional_value(lt_reversed_args_lambda, rhs, lhs);
}

} // namespace impl_

template<class FunT, class T, class... Args>
decltype(auto) call_fun_on_optional_value(FunT&& fun, const Optional<T>& opt, Args&&... args) {
  switch (opt.value_state()) {
  case OptionalState::has_value:
    return fun(opt.val(), std::forward<Args>(args)...);
  case OptionalState::false_value:
    return fun(false, std::forward<Args>(args)...);
  case OptionalState::null_value:
    return fun(mixed{}, std::forward<Args>(args)...);
  default:
    __builtin_unreachable();
  }
}

template<class FunT, class T, class... Args>
decltype(auto) call_fun_on_optional_value(FunT&& fun, Optional<T>&& opt, Args&&... args) {
  switch (opt.value_state()) {
  case OptionalState::has_value:
    return fun(std::move(opt.val()), std::forward<Args>(args)...);
  case OptionalState::false_value:
    return fun(false, std::forward<Args>(args)...);
  case OptionalState::null_value:
    return fun(mixed{}, std::forward<Args>(args)...);
  default:
    __builtin_unreachable();
  }
}
