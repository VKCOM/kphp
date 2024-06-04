#pragma once

#include "runtime-core/utils/migration-php8.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

namespace impl_ {

template<class T1, class T2>
inline bool optional_eq2_impl(const Optional<T1> &lhs, const T2 &rhs);
template<class T1, class T2>
inline bool optional_equals_impl(const Optional<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
inline bool optional_lt_impl(const Optional<T1> &lhs, const T2 &rhs);
template<class T1, class T2>
inline enable_if_t_is_not_optional<T1, bool>  optional_lt_impl(const T1 &lhs, const Optional<T2> &rhs);

} // namespace impl_

template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, const Optional<T> &opt, Args &&...args);
template<class FunT, class T, class... Args>
inline decltype(auto) call_fun_on_optional_value(FunT &&fun, Optional<T> &&opt, Args &&...args);


template<class T, class U>
inline enable_if_one_of_types_is_unknown<T, U> eq2(const T &, const U &) {
  php_assert ("Comparison of Unknown" && 0);
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

template<typename Allocator>
inline bool eq2(const __string<Allocator> &lhs, const __string<Allocator> &rhs) {
  return compare_strings_php_order(lhs, rhs) == 0;
}

template<typename Allocator>
inline bool eq2(const __mixed<Allocator> &lhs, const __mixed<Allocator> &rhs) {
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

template<typename Allocator>
inline bool eq2(bool lhs, const __string<Allocator> &rhs) {
  return lhs == rhs.to_bool();
}

template<typename Allocator>
inline bool eq2(const __string<Allocator> &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

// see https://www.php.net/manual/en/migration80.incompatible.php#migration80.incompatible.core.string-number-comparision
template <typename T, typename Allocator>
inline bool eq2_number_string_as_php8(T lhs, const __string<Allocator> &rhs) {
  auto rhs_float = 0.0;
  const auto rhs_is_string_number = rhs.try_to_float(&rhs_float);

  if (rhs_is_string_number) {
    return eq2(lhs, rhs_float);
  } else {
    return eq2(string(lhs), rhs);
  }
}

template<typename Allocator>
inline bool eq2(int64_t lhs, const __string<Allocator> &rhs) {
  const auto php7_result = eq2(lhs, rhs.to_float());
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = eq2_number_string_as_php8(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning("Comparison (operator ==) results in PHP 7 and PHP 8 are different for %" PRIi64 " and \"%s\" (PHP7: %s, PHP8: %s)",
                lhs,
                rhs.c_str(),
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}

template<typename Allocator>
inline bool eq2(const __string<Allocator> &lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

template<typename Allocator>
inline bool eq2(double lhs, const __string<Allocator> &rhs) {
  const auto php7_result = lhs == rhs.to_float();
  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_COMPARISON_FLAG) {
    const auto php8_result = eq2_number_string_as_php8(lhs, rhs);
    if (php7_result == php8_result) {
      return php7_result;
    }

    php_warning("Comparison (operator ==) results in PHP 7 and PHP 8 are different for %lf and \"%s\" (PHP7: %s, PHP8: %s)",
                lhs,
                rhs.c_str(),
                php7_result ? "true" : "false",
                php8_result ? "true" : "false");
  }

  return php7_result;
}

template<typename Allocator>
inline bool eq2(const __string<Allocator> &lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(bool lhs, const __array<T, Allocator> &rhs) {
  return lhs == !rhs.empty();
}
template<class T, typename Allocator>
inline bool eq2(const __array<T, Allocator> &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(int64_t, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator == (int and array)");
  return false;
}
template<class T, typename Allocator>
inline bool eq2(const __array<T, Allocator> &lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(double, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator == (float and array)");
  return false;
}
template<class T, typename Allocator>
inline bool eq2(const __array<T, Allocator> &lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(const __string<Allocator> &, const __array<T, Allocator> &) {
  php_warning("Unsupported operand types for operator == (__runtime_core::string<Allocator> and array)");
  return false;
}
template<class T, typename Allocator>
inline bool eq2(const __array<T, Allocator> &lhs, const __string<Allocator> &rhs) {
  return eq2(rhs, lhs);
}

template<class TupleLhsT, class TupleRhsT, size_t ...Indices>
inline bool eq2(const TupleLhsT &lhs, const TupleRhsT &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, eq2(std::get<Indices>(lhs), std::get<Indices>(rhs))...);
}

template<class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, const std::tuple<Args...> &rhs) {
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class ...Args, class ...Args2>
inline std::enable_if_t<sizeof...(Args) == sizeof...(Args2), bool> eq2(const std::tuple<Args...> &lhs, const std::tuple<Args2...> &rhs) {
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class ...Args, class ...Args2>
inline std::enable_if_t<sizeof...(Args) != sizeof...(Args2), bool> eq2(const std::tuple<Args...> &, const std::tuple<Args2...> &) {
  return false;
}

template<typename Allocator, class TupleT, class Arg, size_t ...Indices>
inline bool eq2(const TupleT &lhs, const __array<Arg, Allocator>  &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, eq2(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<typename Allocator, class Arg, class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, const __array<Arg, Allocator>  &rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().size) {
    return false;
  }
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<typename Allocator, class Arg, class ...Args>
inline bool eq2(const __array<Arg, Allocator>  &lhs, const std::tuple<Args...> &rhs) {
  return eq2(rhs, lhs);
}

template<class ...Args>
inline bool eq2(bool lhs, const std::tuple<Args...> &) {
  return lhs;
}
template<class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<class T, class ...Args>
inline bool eq2(const T &, const std::tuple<Args...> &) {
  return false;
}
template<class T, class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, const T &rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(const __class_instance<T, Allocator> &lhs, const __class_instance<T, Allocator> &rhs) {
  // can be called implicitly from in_array, for example
  return lhs == rhs;
}

template<typename Allocator>
inline bool eq2(bool lhs, const __mixed<Allocator>  &rhs) {
  return lhs == rhs.to_bool();
}

template<typename Allocator>
inline bool eq2(const __mixed<Allocator>  &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<typename Allocator>
inline bool eq2(double lhs, const __mixed<Allocator>  &rhs) {
  switch (rhs.get_type()) {
    case __runtime_core::mixed<Allocator> ::type::NUL:
      return eq2(lhs, 0.0);
    case __runtime_core::mixed<Allocator> ::type::BOOLEAN:
      return eq2(lhs, rhs.as_bool());
    case __runtime_core::mixed<Allocator> ::type::INTEGER:
      return eq2(lhs, rhs.as_int());
    case __runtime_core::mixed<Allocator> ::type::FLOAT:
      return eq2(lhs, rhs.as_double());
    case __runtime_core::mixed<Allocator> ::type::STRING:
      return eq2(lhs, rhs.as_string());
    case __runtime_core::mixed<Allocator> ::type::ARRAY:
      php_warning("Unsupported operand types for operator == (float and array)");
      return false;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
inline bool eq2(const __mixed<Allocator>  &lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<typename Allocator>
inline bool eq2(int64_t lhs, const __mixed<Allocator>  &rhs) {
  switch (rhs.get_type()) {
    case __runtime_core::mixed<Allocator> ::type::NUL:
      return eq2(lhs, int64_t{0});
    case __runtime_core::mixed<Allocator> ::type::BOOLEAN:
      return eq2(lhs, rhs.as_bool());
    case __runtime_core::mixed<Allocator> ::type::INTEGER:
      return eq2(lhs, rhs.as_int());
    case __runtime_core::mixed<Allocator> ::type::FLOAT:
      return eq2(lhs, rhs.as_double());
    case __runtime_core::mixed<Allocator> ::type::STRING:
      return eq2(lhs, rhs.as_string());
    case __runtime_core::mixed<Allocator> ::type::ARRAY:
      php_warning("Unsupported operand types for operator == (int and array)");
      return false;
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
inline bool eq2(const __mixed<Allocator>  &lhs, int64_t rhs) {
  return eq2(rhs, lhs);
}

template<typename Allocator>
inline bool eq2(const __string<Allocator> &lhs, const __runtime_core::mixed<Allocator>  &rhs) {
  return eq2(__runtime_core::mixed<Allocator> (lhs), rhs);
}

template<typename Allocator>
inline bool eq2(const __mixed<Allocator>  &lhs, const __runtime_core::string<Allocator> &rhs) {
  return eq2(rhs, lhs);
}

template<class T, typename Allocator>
inline bool eq2(const __array<T, Allocator> &lhs, const __mixed<Allocator>  &rhs) {
  if (likely (rhs.is_array())) {
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
template<class T, typename Allocator>
inline bool eq2(const __mixed<Allocator>  &lhs, const __array<T, Allocator> &rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1> &lhs, const T2 &rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool eq2(const T1 &lhs, const Optional<T2> &rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1> &lhs, const Optional<T2> &rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}

template<class T>
inline bool eq2(const Optional<T> &lhs, const Optional<T> &rhs) {
  return impl_::optional_eq2_impl(lhs, rhs);
}

template<class T1, class T2, typename Allocator>
inline bool eq2(const __array<T1, Allocator> &lhs, const __array<T2, Allocator> &rhs) {
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
inline bool eq2(const T &lhs, int32_t rhs);

template<class T>
inline bool eq2(int32_t lhs, const T &rhs);

template<class T>
inline bool eq2(const T &lhs, int32_t rhs) {
  return eq2(lhs, int64_t{rhs});
}

template<class T>
inline bool eq2(int32_t lhs, const T &rhs) {
  return eq2(int64_t{lhs}, rhs);
}

template<class T, class U>
inline enable_if_one_of_types_is_unknown<T, U> equals(const T &lhs, const U &rhs) {
  return eq2(lhs, rhs);
}
template<class T, class U>
inline disable_if_one_of_types_is_unknown<T, U> equals(const T &, const U &) {
  return false;
}
template<class T>
inline disable_if_one_of_types_is_unknown<T, T> equals(const T &lhs, const T &rhs) {
  return lhs == rhs;
}


template<class T, typename Allocator>
inline bool equals(const __array<T, Allocator> &lhs, const __array<T, Allocator> &rhs) {
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
template<class T1, class T2, typename Allocator>
inline bool equals(const __array<T1, Allocator> &lhs, const __array<T2, Allocator> &rhs) {
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

template<typename Allocator>
inline bool equals(bool lhs, const __mixed<Allocator>  &rhs) {
  return rhs.is_bool() && equals(lhs, rhs.as_bool());
}
template<typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, bool rhs) {
  return equals(rhs, lhs);
}

template<typename Allocator>
inline bool equals(int64_t lhs, const __mixed<Allocator>  &rhs) {
  return rhs.is_int() && equals(lhs, rhs.as_int());
}
template<typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, int64_t rhs) {
  return equals(rhs, lhs);
}

template<typename Allocator>
inline bool equals(double lhs, const __mixed<Allocator>  &rhs) {
  return rhs.is_float() && equals(lhs, rhs.as_double());
}
template<typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, double rhs) {
  return equals(rhs, lhs);
}

template<typename Allocator>
inline bool equals(const __string<Allocator> &lhs, const __mixed<Allocator>  &rhs) {
  return rhs.is_string() && equals(lhs, rhs.as_string());
}
template<typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, const __string<Allocator> &rhs) {
  return equals(rhs, lhs);
}

template<class T, typename Allocator>
inline bool equals(const __array<T, Allocator> &lhs, const __mixed<Allocator>  &rhs) {
  return rhs.is_array() && equals(lhs, rhs.as_array());
}
template<class T, typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, const __array<T, Allocator> &rhs) {
  return equals(rhs, lhs);
}
template<typename Allocator>
inline bool equals(const __mixed<Allocator>  &lhs, const __mixed<Allocator>  &rhs) {
  if (lhs.get_type() != rhs.get_type()) {
    return false;
  }

  switch (lhs.get_type()) {
    case __mixed<Allocator> ::type::NUL:
      return true;
    case __mixed<Allocator> ::type::BOOLEAN:
      return equals(lhs.as_bool(), rhs.as_bool());
    case __mixed<Allocator> ::type::INTEGER:
      return equals(lhs.as_int(), rhs.as_int());
    case __mixed<Allocator> ::type::FLOAT:
      return equals(lhs.as_double(), rhs.as_double());
    case __mixed<Allocator> ::type::STRING:
      return equals(lhs.as_string(), rhs.as_string());
    case __mixed<Allocator> ::type::ARRAY:
      return equals(lhs.as_array(), rhs.as_array());
    default:
      __builtin_unreachable();
  }
}

template<class T, typename Allocator>
inline bool equals(const __class_instance<T, Allocator> &lhs, const __class_instance<T, Allocator> &rhs) {
  return lhs == rhs;
}

template<class TupleLhsT, class TupleRhsT, size_t ...Indices>
inline bool equals(const TupleLhsT &lhs, const TupleRhsT &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, equals(std::get<Indices>(lhs), std::get<Indices>(rhs))...);
}

template<class ...Args>
inline bool equals(const std::tuple<Args...> &lhs, const std::tuple<Args...> &rhs) {
  return equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<class ...Args, class ...Args2>
inline bool equals(const std::tuple<Args...> &lhs, const std::tuple<Args2...> &rhs) {
  return sizeof...(Args) == sizeof...(Args2) &&
         equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}

template<typename Allocator, class TupleT, class Arg, size_t ...Indices>
inline bool equals(const TupleT &lhs, const __array<Arg, Allocator>  &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, equals(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<typename Allocator, class Arg, class ...Args>
inline bool equals(const std::tuple<Args...> &lhs, const __array<Arg, Allocator>  &rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().size) {
    return false;
  }
  return equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<typename Allocator, class Arg, class ...Args>
inline bool equals(const __array<Arg, Allocator>  &lhs, const std::tuple<Args...> &rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2, typename Allocator>
inline std::enable_if_t<std::is_base_of<T1, T2>{} || std::is_base_of<T2, T1>{}, bool>
equals(const __class_instance<T1, Allocator> &lhs, const __class_instance<T2, Allocator> &rhs) {
  return dynamic_cast<void *>(lhs.get()) == dynamic_cast<void *>(rhs.get());
}
template<class T1, class T2, typename Allocator>
inline std::enable_if_t<!std::is_base_of<T1, T2>{} && !std::is_base_of<T2, T1>{}, bool>
equals(const __class_instance<T1, Allocator> &, const __class_instance<T2, Allocator> &) {
  return false;
}


template<class T1, class T2>
inline bool equals(const Optional<T1> &lhs, const T2 &rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool equals(const T1 &lhs, const Optional<T2> &rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2>
inline bool equals(const Optional<T1> &lhs, const Optional<T2> &rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}

template<class T>
inline bool equals(const Optional<T> &lhs, const Optional<T> &rhs) {
  return impl_::optional_equals_impl(lhs, rhs);
}

template<class T>
inline bool equals(const T &lhs, int32_t rhs);

template<class T>
inline bool equals(int32_t lhs, const T &rhs);

template<class T>
inline bool equals(const T &lhs, int32_t rhs) {
  return equals(lhs, int64_t{rhs});
}

template<class T>
inline bool equals(int32_t lhs, const T &rhs) {
  return equals(int64_t{lhs}, rhs);
}

inline bool lt(const bool &lhs, const bool &rhs) {
  return lhs < rhs;
}
template<class T1, class T2>
inline bool lt(const T1 &lhs, const T2 &rhs) {
  return lhs < rhs;
}

template<class T2>
inline bool lt(const bool &lhs, const T2 &rhs) {
  return lhs < f$boolval(rhs);
}
template<class T1>
inline bool lt(const T1 &lhs, const bool &rhs) {
  return f$boolval(lhs) < rhs;
}

template<class T1, class T2>
inline bool lt(const Optional<T1> &lhs, const T2 &rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool lt(const T1 &lhs, const Optional<T2> &rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}

template<class T>
inline bool lt(const Optional<T> &lhs, const Optional<T> &rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool lt(const Optional<T1> &lhs, const Optional<T2> &rhs) {
  return impl_::optional_lt_impl(lhs, rhs);
}

template<class T>
inline bool lt(const bool &lhs, const Optional<T> &rhs) {
  return lt(lhs, f$boolval(rhs));
}
template<class T>
inline bool lt(const Optional<T> &lhs, const bool &rhs) {
  return lt(f$boolval(lhs), rhs);
}

template<class T1, class T2, typename Allocator>
inline bool lt(const __array<T1, Allocator> &lhs, const __array<T2, Allocator> &rhs) {
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
inline bool leq(const T1 &lhs, const T2 &rhs) {
  // (lhs <= rhs) === (!(rhs < lhs))
  return !lt(rhs, lhs);
}

namespace impl_ {

template<class T1, class T2>
bool optional_eq2_impl(const Optional<T1> &lhs, const T2 &rhs) {
  auto eq2_lambda = [](const auto &l, const auto &r) { return eq2(r, l); };
  return call_fun_on_optional_value(eq2_lambda, lhs, rhs);
}

template<class T1, class T2>
bool optional_equals_impl(const Optional<T1> &lhs, const T2 &rhs) {
  auto equals_lambda = [](const auto &l, const auto &r) {
    // if (is_null(lhs)) { return is_null(r); }
    // else return equals(r, l); - parameters are swapped to cope with Optional on right side
    return is_instance_of_const_ref<decltype(l), __mixed>::value ? f$is_null(r) : equals(r, l);
  };
  return call_fun_on_optional_value(equals_lambda, lhs, rhs);
}

template<class T1, class T2>
inline bool optional_lt_impl(const Optional<T1> &lhs, const T2 &rhs) {
  auto lt_lambda = [](const auto &l, const auto &r) { return lt(l, r);};
  return call_fun_on_optional_value(lt_lambda, lhs, rhs);
}

template<class T1, class T2>
inline enable_if_t_is_not_optional<T1, bool>  optional_lt_impl(const T1 &lhs, const Optional<T2> &rhs) {
  auto lt_reversed_args_lambda = [](const auto &l, const auto &r) { return lt(r, l);};
  return call_fun_on_optional_value(lt_reversed_args_lambda, rhs, lhs);
}

} // namespace impl_

namespace __runtime_core {
template<typename Allocator, class FunT, class T, class... Args>
decltype(auto) call_fun_on_optional_value(FunT &&fun, const Optional<T> &opt, Args &&...args) {
  switch (opt.value_state()) {
    case OptionalState::has_value:
      return fun(opt.val(), std::forward<Args>(args)...);
    case OptionalState::false_value:
      return fun(false, std::forward<Args>(args)...);
    case OptionalState::null_value:
      return fun(__mixed<Allocator>{}, std::forward<Args>(args)...);
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator, class FunT, class T, class... Args>
decltype(auto) call_fun_on_optional_value(FunT &&fun, Optional<T> &&opt, Args &&...args) {
  switch (opt.value_state()) {
    case OptionalState::has_value:
      return fun(std::move(opt.val()), std::forward<Args>(args)...);
    case OptionalState::false_value:
      return fun(false, std::forward<Args>(args)...);
    case OptionalState::null_value:
      return fun(__mixed<Allocator>{}, std::forward<Args>(args)...);
    default:
      __builtin_unreachable();
  }
}
}
