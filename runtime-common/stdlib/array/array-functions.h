// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>
#include <tuple>

#include "common/type_traits/function_traits.h"
#include "common/type_traits/list_of_types.h"
#include "runtime-common/core/runtime-core.h"

inline constexpr int64_t SORT_REGULAR = 0;
inline constexpr int64_t SORT_NUMERIC = 1;
inline constexpr int64_t SORT_STRING = 2;

namespace array_functions_impl_ {

string implode_string_vector(const string &s, const array<string> &a) noexcept;

template<class T, class F>
auto transform_to_vector(const array<T> &a, const F &op) noexcept {
  array<typename vk::function_traits<F>::ResultType> result(array_size(a.count(), true));
  for (const auto &it : a) {
    result.push_back(op(it));
  }
  return result;
}

} // namespace array_functions_impl_

template<class T>
string f$implode(const string &s, const array<T> &a) noexcept {
  int64_t count = a.count();
  if (count == 1) {
    return f$strval(a.begin().get_value());
  }
  if (count == 0) {
    return string{};
  }

  if constexpr (std::is_same_v<T, string>) {
    if (a.is_vector()) {
      // traversing a flat vector is fast enough, so we can do it twice:
      // calculate the total result length and append the result
      // right into the dst string;
      // (sometimes we don't even need to traverse the array for result_size, see int array overloading)
      // this way we avoid x2 data bytes copying (into SB and then into the result string);
      return array_functions_impl_::implode_string_vector(s, a);
    }
  }

  // fallback to the generic iterator + string_buffer solution

  string_buffer &SB = RuntimeContext::get().static_SB;
  SB.clean();

  auto it = a.begin();
  auto it_end = a.end();
  if (it != it_end) {
    SB << it.get_value();
    ++it;
  }
  while (it != it_end) {
    SB << s << it.get_value();
    ++it;
  }

  return SB.str();
}

array<string> explode(char delimiter, const string &str, int64_t limit = std::numeric_limits<int64_t>::max()) noexcept;

array<string> f$explode(const string &delimiter, const string &str, int64_t limit = std::numeric_limits<int64_t>::max()) noexcept;

string f$_explode_nth(const string &delimiter, const string &str, int64_t index) noexcept;

string f$_explode_1(const string &delimiter, const string &str) noexcept;

std::tuple<string, string> f$_explode_tuple2(const string &delimiter, const string &str, int64_t mask, int64_t limit = 2 + 1) noexcept;

std::tuple<string, string, string> f$_explode_tuple3(const string &delimiter, const string &str, int64_t mask, int64_t limit = 3 + 1) noexcept;

std::tuple<string, string, string, string> f$_explode_tuple4(const string &delimiter, const string &str, int64_t mask, int64_t limit = 4 + 1) noexcept;

template<class T>
array<T> f$array_reverse(const array<T> &a, bool preserve_keys = false) noexcept {
  array<T> result(a.size());

  const auto first = a.begin();
  for (auto it = a.end(); it != first;) {
    --it;

    if (!preserve_keys) {
      result.push_back(it);
    } else {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_intersect(const array<T> &a1, const array<T1> &a2) noexcept {
  array<T> result(a1.size().min(a2.size()));

  array<int64_t> values(array_size(a2.count(), false));
  for (const auto &it : a2) {
    values.set_value(f$strval(it.get_value()), 1);
  }

  for (const auto &it : a1) {
    if (values.has_key(f$strval(it.get_value()))) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_intersect_key(const array<T> &a1, const array<T1> &a2) noexcept {
  array<T> result(a1.size().min(a2.size()));
  for (const auto &it : a1) {
    if (a2.has_key(it.get_key())) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T>
bool f$array_key_exists(int64_t int_key, const array<T> &a) noexcept {
  return a.has_key(int_key);
}

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a) noexcept {
  return a.has_key(string_key);
}

template<class T>
bool f$array_key_exists(const mixed &v, const array<T> &a) noexcept {
  return (v.is_int() || v.is_string() || v.is_null()) && a.has_key(v);
}

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a) noexcept {
  return f$array_key_exists(mixed(v), a);
}

template<class K, class T, class = vk::enable_if_in_list<K, vk::list_of_types<double, bool>>>
constexpr bool f$array_key_exists(K /*key*/, const array<T> & /*a*/) noexcept {
  return false;
}

template<class T>
array<typename array<T>::key_type> f$array_keys(const array<T> &a) noexcept {
  using Iterator = typename array<T>::const_iterator;
  return array_functions_impl_::transform_to_vector(a, [](Iterator it) { return it.get_key(); });
}

template<class T>
array<string> f$array_keys_as_strings(const array<T> &a) noexcept {
  using Iterator = typename array<T>::const_iterator;
  return array_functions_impl_::transform_to_vector(a, [](Iterator it) { return it.get_key().to_string(); });
}

template<class T>
array<int64_t> f$array_keys_as_ints(const array<T> &a) noexcept {
  using Iterator = typename array<T>::const_iterator;
  return array_functions_impl_::transform_to_vector(a, [](Iterator it) { return it.get_key().to_int(); });
}

template<class T>
array<T> f$array_unique(const array<T> &a, int64_t flags = SORT_STRING) noexcept {
  array<int64_t> values(array_size(a.count(), false));
  array<T> result(a.size());

  auto lookup = [flags, &values](const auto &value) -> int64_t & {
    switch (flags) {
      case SORT_REGULAR:
        return values[value];
      case SORT_NUMERIC:
        return values[f$intval(value)];
      case SORT_STRING:
        return values[f$strval(value)];
      default:
        php_warning("Unsupported flags in function array_unique");
        return values[f$strval(value)];
    }
  };

  for (const auto &it : a) {
    const T &value = it.get_value();
    int64_t &cnt = lookup(value);
    if (!cnt) {
      cnt = 1;
      result.set_value(it);
    }
  }
  return result;
}

template<class T>
array<T> f$array_values(const array<T> &a) noexcept {
  using Iterator = typename array<T>::const_iterator;
  return array_functions_impl_::transform_to_vector(a, [](Iterator it) { return it.get_value(); });
}

template<class T>
T f$array_shift(array<T> &a) noexcept {
  return a.shift();
}

template<class T, class T1>
int64_t f$array_unshift(array<T> &a, const T1 &val) noexcept {
  return a.unshift(val);
}

template<class T>
T f$array_merge(const T &a1) noexcept {
  T result(a1.size());
  result.merge_with(a1);
  return result;
}

template<class T>
T f$array_merge(const T &a1, const T &a2) noexcept {
  T result(a1.size() + a2.size());
  result.merge_with(a1);
  result.merge_with(a2);
  return result;
}

template<class T>
T f$array_merge(const T &a1, const T &a2, const T &a3, const T &a4 = {}, const T &a5 = {}, const T &a6 = {}, const T &a7 = {}, const T &a8 = {},
                const T &a9 = {}, const T &a10 = {}, const T &a11 = {}, const T &a12 = {}) noexcept {
  T result(a1.size() + a2.size() + a3.size() + a4.size() + a5.size() + a6.size() + a7.size() + a8.size() + a9.size() + a10.size() + a11.size() + a12.size());
  result.merge_with(a1);
  result.merge_with(a2);
  result.merge_with(a3);
  result.merge_with(a4);
  result.merge_with(a5);
  result.merge_with(a6);
  result.merge_with(a7);
  result.merge_with(a8);
  result.merge_with(a9);
  result.merge_with(a10);
  result.merge_with(a11);
  result.merge_with(a12);
  return result;
}

template<class T, class T1>
void f$array_merge_into(T &a, const T1 &another_array) noexcept {
  a.merge_with(another_array);
}

template<class T>
void f$array_reserve(array<T> &a, int64_t int_size, int64_t string_size, bool make_vector_if_possible = true) noexcept {
  a.reserve(int_size + string_size, make_vector_if_possible);
}

template<class T>
void f$array_reserve_vector(array<T> &a, int64_t size) noexcept {
  a.reserve(size, true);
}

template<class T>
void f$array_reserve_map_int_keys(array<T> &a, int64_t size) noexcept {
  a.reserve(size, false);
}

template<class T>
void f$array_reserve_map_string_keys(array<T> &a, int64_t size) noexcept {
  a.reserve(size, false);
}

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base) noexcept {
  auto size_info = base.size();
  f$array_reserve(a, size_info.size, size_info.is_vector);
}

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict = false) noexcept {
  for (const auto &it : a) {
    if (strict ? equals(it.get_value(), val) : eq2(it.get_value(), val)) {
      return it.get_key();
    }
  }

  return typename array<T>::key_type(false);
}

template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict = false) noexcept {
  if (!strict) {
    for (const auto &it : a) {
      if (eq2(it.get_value(), value)) {
        return true;
      }
    }
  } else {
    for (const auto &it : a) {
      if (equals(it.get_value(), value)) {
        return true;
      }
    }
  }
  return false;
}

template<class T1, class T2>
int64_t f$array_push(array<T1> &a, const T2 &val) noexcept {
  a.push_back(val);
  return a.count();
}

template<class T1, class T2, class T3>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3) noexcept {
  a.push_back(val2);
  a.push_back(val3);
  return a.count();
}

template<class T1, class T2, class T3, class T4>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4) noexcept {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  return a.count();
}

template<class T1, class T2, class T3, class T4, class T5>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5) noexcept {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  a.push_back(val5);
  return a.count();
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6) noexcept {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  a.push_back(val5);
  a.push_back(val6);
  return a.count();
}

template<class T>
T f$array_pop(array<T> &a) noexcept {
  return a.pop();
}

template<class T>
T f$array_unset(array<T> &arr, int64_t key) noexcept {
  return arr.unset(key);
}

template<class T>
T f$array_unset(array<T> &arr, const string &key) noexcept {
  return arr.unset(key);
}

template<class T>
T f$array_unset(array<T> &arr, const mixed &key) noexcept {
  return arr.unset(key);
}

template<class T>
bool f$array_is_vector(const array<T> &a) noexcept {
  return a.is_vector();
}

template<class T>
bool f$array_is_list(const array<T> &a) noexcept {
  // is_vector() is fast, but not enough;
  // is_pseudo_vector() doesn't cover is_vector() case,
  // so we need to call both of them to get the precise and PHP-compatible answer
  return a.is_vector() || a.is_pseudo_vector();
}
