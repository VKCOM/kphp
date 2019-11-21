#pragma once

namespace {

template<class T, class U>
struct one_of_is_unknown
  : std::integral_constant<bool, std::is_same<T, Unknown>::value || std::is_same<U, Unknown>::value> {
};

template<class T, class U>
using enable_if_one_of_types_is_unknown = typename std::enable_if<one_of_is_unknown<T, U>::value, bool>::type;

template<class T, class U>
using disable_if_one_of_types_is_unknown = typename std::enable_if<!one_of_is_unknown<T, U>::value && !(is_class_instance<T>{} && is_class_instance<U>{}), bool>::type;

template<class T1, class T2>
bool optional_eq2_impl(const Optional<T1> &lhs, const T2 &rhs);

template<class T1, class T2>
bool optional_equals_impl(const Optional<T1> &lhs, const T2 &rhs);
} // namespace

template<class T, class U>
inline enable_if_one_of_types_is_unknown<T, U> eq2(const T &, const U &) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

inline bool eq2(bool lhs, bool rhs) {
  return lhs == rhs;
}
inline bool eq2(int lhs, int rhs) {
  return lhs == rhs;
}
inline bool eq2(double lhs, double rhs) {
  return lhs == rhs;
}
inline bool eq2(const string &lhs, const string &rhs) {
  return compare_strings_php_order(lhs, rhs) == 0;
}
template<class T>
inline bool eq2(const array<T> &lhs, const array<T> &rhs) {
  if (rhs.count() != lhs.count()) {
    return false;
  }

  for (typename array<T>::const_iterator rhs_it = rhs.begin(); rhs_it != rhs.end(); ++rhs_it) {
    if (!lhs.has_key(rhs_it) || !eq2(lhs.get_value(rhs_it), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}
inline bool eq2(const var &lhs, const var &rhs) {
  if (unlikely (lhs.is_string())) {
    if (likely (rhs.is_string())) {
      return eq2(lhs.as_string(), rhs.as_string());
    } else if (unlikely (rhs.is_null())) {
      return lhs.as_string().empty();
    }
  } else if (unlikely (rhs.is_string())) {
    if (unlikely (lhs.is_null())) {
      return rhs.as_string().empty();
    }
  }
  if (lhs.is_bool() || lhs.is_null() || rhs.is_bool() || rhs.is_null()) {
    return lhs.to_bool() == rhs.to_bool();
  }

  if (unlikely (lhs.is_array() || rhs.is_array())) {
    if (likely (lhs.is_array() && rhs.is_array())) {
      return eq2(lhs.as_array(), rhs.as_array());
    }

    php_warning("Unsupported operand types for operator == (%s and %s)", lhs.get_type_c_str(), rhs.get_type_c_str());
    return false;
  }

  return lhs.to_float() == rhs.to_float();
}

inline bool eq2(bool lhs, int rhs) {
  return lhs != !rhs;
}
inline bool eq2(int lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(bool lhs, double rhs) {
  return lhs != (rhs == 0.0);
}
inline bool eq2(double lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(int lhs, double rhs) {
  return lhs == rhs;
}
inline bool eq2(double lhs, int rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(bool lhs, const string &rhs) {
  return lhs == rhs.to_bool();
}
inline bool eq2(const string &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(int lhs, const string &rhs) {
  return lhs == rhs.to_float();
}
inline bool eq2(const string &lhs, int rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(double lhs, const string &rhs) {
  return lhs == rhs.to_float();
}
inline bool eq2(const string &lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(bool lhs, const array<T> &rhs) {
  return lhs == !rhs.empty();
}
template<class T>
inline bool eq2(const array<T> &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(int, const array<T> &) {
  php_warning("Unsupported operand types for operator == (int and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T> &lhs, int rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(double, const array<T> &) {
  php_warning("Unsupported operand types for operator == (float and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T> &lhs, double rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(const string &, const array<T> &) {
  php_warning("Unsupported operand types for operator == (string and array)");
  return false;
}
template<class T>
inline bool eq2(const array<T> &lhs, const string &rhs) {
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

template<class TupleT, class Arg, size_t ...Indices>
inline bool eq2(const TupleT &lhs, const array<Arg> &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, eq2(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<class Arg, class ...Args>
inline bool eq2(const std::tuple<Args...> &lhs, const array<Arg> &rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().int_size) {
    return false;
  }
  return eq2(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<class Arg, class ...Args>
inline bool eq2(const array<Arg> &lhs, const std::tuple<Args...> &rhs) {
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

template<class T>
inline bool eq2(const class_instance<T> &lhs, const class_instance<T> &rhs) {
  // can be called implicitly from in_array, for example
  return lhs == rhs;
}

inline bool eq2(bool lhs, const var &rhs) {
  return lhs == rhs.to_bool();
}
inline bool eq2(const var &lhs, bool rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(double lhs, const var &rhs) {
  switch (rhs.get_type()) {
    case var::NULL_TYPE:
      return eq2(lhs, 0.0);
    case var::BOOLEAN_TYPE:
      return eq2(lhs, rhs.as_bool());
    case var::INTEGER_TYPE:
      return eq2(lhs, rhs.as_int());
    case var::FLOAT_TYPE:
      return eq2(lhs, rhs.as_double());
    case var::STRING_TYPE:
      return eq2(lhs, rhs.as_string());
    case var::ARRAY_TYPE:
      php_warning("Unsupported operand types for operator == (float and array)");
      return false;
    default:
      __builtin_unreachable();
  }
}
inline bool eq2(const var &lhs, double rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(int lhs, const var &rhs) {
  return eq2(static_cast<double>(lhs), rhs);
}
inline bool eq2(const var &lhs, int rhs) {
  return eq2(rhs, lhs);
}

inline bool eq2(const string &lhs, const var &rhs) {
  return eq2(var(lhs), rhs);
}
inline bool eq2(const var &lhs, const string &rhs) {
  return eq2(rhs, lhs);
}

template<class T>
inline bool eq2(const array<T> &lhs, const var &rhs) {
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
template<class T>
inline bool eq2(const var &lhs, const array<T> &rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1> &lhs, const T2 &rhs) {
  return optional_eq2_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool eq2(const T1 &lhs, const Optional<T2> &rhs) {
  return eq2(rhs, lhs);
}

template<class T1, class T2>
inline bool eq2(const Optional<T1> &lhs, const Optional<T2> &rhs) {
  return optional_eq2_impl(lhs, rhs);
}

template<class T>
inline bool eq2(const Optional<T> &lhs, const Optional<T> &rhs) {
  return optional_eq2_impl(lhs, rhs);
}

template<class T1, class T2>
inline bool eq2(const array<T1> &lhs, const array<T2> &rhs) {
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

template<class T>
inline bool equals(const array<T> &lhs, const array<T> &rhs) {
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
inline bool equals(const array<T1> &lhs, const array<T2> &rhs) {
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


inline bool equals(const var &lhs, const var &rhs) {
  if (lhs.get_type() != rhs.get_type()) {
    return false;
  }

  switch (lhs.get_type()) {
    case var::NULL_TYPE:
      return true;
    case var::BOOLEAN_TYPE:
      return equals(lhs.as_bool(), rhs.as_bool());
    case var::INTEGER_TYPE:
      return equals(lhs.as_int(), rhs.as_int());
    case var::FLOAT_TYPE:
      return equals(lhs.as_double(), rhs.as_double());
    case var::STRING_TYPE:
      return equals(lhs.as_string(), rhs.as_string());
    case var::ARRAY_TYPE:
      return equals(lhs.as_array(), rhs.as_array());
    default:
      __builtin_unreachable();
  }
}

inline bool equals(bool lhs, const var &rhs) {
  return rhs.is_bool() && equals(lhs, rhs.as_bool());
}
inline bool equals(const var &lhs, bool rhs) {
  return equals(rhs, lhs);
}

inline bool equals(int lhs, const var &rhs) {
  return rhs.is_int() && equals(lhs, rhs.as_int());
}
inline bool equals(const var &lhs, int rhs) {
  return equals(rhs, lhs);
}

inline bool equals(double lhs, const var &rhs) {
  return rhs.is_float() && equals(lhs, rhs.as_double());
}
inline bool equals(const var &lhs, double rhs) {
  return equals(rhs, lhs);
}

inline bool equals(const string &lhs, const var &rhs) {
  return rhs.is_string() && equals(lhs, rhs.as_string());
}
inline bool equals(const var &lhs, const string &rhs) {
  return equals(rhs, lhs);
}

template<class T>
inline bool equals(const array<T> &lhs, const var &rhs) {
  return rhs.is_array() && equals(lhs, rhs.as_array());
}
template<class T>
inline bool equals(const var &lhs, const array<T> &rhs) {
  return equals(rhs, lhs);
}

template<class T>
inline bool equals(const class_instance<T> &lhs, const class_instance<T> &rhs) {
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

template<class TupleT, class Arg, size_t ...Indices>
inline bool equals(const TupleT &lhs, const array<Arg> &rhs, std::index_sequence<Indices...>) {
  return vk::all_of_equal(true, equals(std::get<Indices>(lhs), rhs.get_value(Indices))...);
}

template<class Arg, class ...Args>
inline bool equals(const std::tuple<Args...> &lhs, const array<Arg> &rhs) {
  if (!rhs.is_vector() || sizeof...(Args) != rhs.size().int_size) {
    return false;
  }
  return equals(lhs, rhs, std::make_index_sequence<sizeof...(Args)>{});
}
template<class Arg, class ...Args>
inline bool equals(const array<Arg> &lhs, const std::tuple<Args...> &rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2>
inline std::enable_if_t<std::is_base_of<T1, T2>{} || std::is_base_of<T2, T1>{}, bool> equals(const class_instance<T1> &lhs, const class_instance<T2> &rhs) {
  return dynamic_cast<void *>(lhs.get()) == dynamic_cast<void *>(rhs.get());
}

template<class T1, class T2>
inline std::enable_if_t<!std::is_base_of<T1, T2>{} && !std::is_base_of<T2, T1>{}, bool>  equals(const class_instance<T1> &, const class_instance<T2> &) {
  return false;
}

namespace {

template<class T1, class T2>
bool optional_eq2_impl(const Optional<T1> &lhs, const T2 &rhs) {
  auto eq2_lambda = [](const auto &l, const auto &r) { return eq2(r, l); };
  return call_fun_on_optional_value(eq2_lambda, lhs, rhs);
}

template<class T1, class T2>
bool optional_equals_impl(const Optional<T1> &lhs, const T2 &rhs) {
  auto equals_lambda = [](const auto &l, const auto &r) { return equals(r, l); };
  return call_fun_on_optional_value(equals_lambda, lhs, rhs);
}

} // namespace

template<class T1, class T2>
inline bool equals(const Optional<T1> &lhs, const T2 &rhs) {
  return optional_equals_impl(lhs, rhs);
}
template<class T1, class T2>
inline bool equals(const T1 &lhs, const Optional<T2> &rhs) {
  return equals(rhs, lhs);
}

template<class T1, class T2>
inline bool equals(const Optional<T1> &lhs, const Optional<T2> &rhs) {
  return optional_equals_impl(lhs, rhs);
}

template<class T>
inline bool equals(const Optional<T> &lhs, const Optional<T> &rhs) {
  return optional_equals_impl(lhs, rhs);
}

template<class T1, class T2>
inline bool neq2(const T1 &lhs, const T2 &rhs) {
  return !eq2(lhs, rhs);
}
