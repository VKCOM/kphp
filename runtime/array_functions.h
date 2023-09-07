// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>
#include <numeric>

#include "common/type_traits/function_traits.h"
#include "common/vector-product.h"

#include "runtime/kphp_core.h"
#include "runtime/math_functions.h"
#include "runtime/string_functions.h"

constexpr int64_t SORT_REGULAR = 0;
constexpr int64_t SORT_NUMERIC = 1;
constexpr int64_t SORT_STRING = 2;

template<class T>
string f$implode(const string &s, const array<T> &a);

array<string> explode(char delimiter, const string &str, int64_t limit = std::numeric_limits<int64_t>::max());

array<string> f$explode(const string &delimiter, const string &str, int64_t limit = std::numeric_limits<int64_t>::max());
string f$_explode_nth(const string &delimiter, const string &str, int64_t index);
string f$_explode_1(const string &delimiter, const string &str);
std::tuple<string, string> f$_explode_tuple2(const string &delimiter, const string &str, int64_t mask, int64_t limit = 2+1);
std::tuple<string, string, string> f$_explode_tuple3(const string &delimiter, const string &str, int64_t mask, int64_t limit = 3+1);
std::tuple<string, string, string, string> f$_explode_tuple4(const string &delimiter, const string &str, int64_t mask, int64_t limit = 4+1);

template<class T>
array<array<T>> f$array_chunk(const array<T> &a, int64_t chunk_size, bool preserve_keys = false);

template<class T>
array<T> f$array_slice(const array<T> &a, int64_t offset, const mixed &length_var = mixed(), bool preserve_keys = false);

template<class T>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length, const array<Unknown> &);

template<class T, class T1 = T>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length = std::numeric_limits<int64_t>::max(), const array<T1> &replacement = array<T1>());

template<class ReturnT, class InputArrayT, class DefaultValueT>
ReturnT f$array_pad(const array<InputArrayT> &a, int64_t size, const DefaultValueT &default_value);

template<class ReturnT, class DefaultValueT>
ReturnT f$array_pad(const array<Unknown> &a, int64_t size, const DefaultValueT &default_value);

template<class T>
array<T> f$array_filter(const array<T> &a) noexcept;

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback) noexcept;

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T> &a, const T1 &callback) noexcept;

template<class T>
T f$array_merge_spread(const T &a1);

template<class T>
T f$array_merge_spread(const T &a1, const T &a2);

template<class T>
T f$array_merge_spread(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(),
                const T &a7 = T(), const T &a8 = T(), const T &a9 = T(),
                const T &a10 = T(), const T &a11 = T(), const T &a12 = T());

template<class T>
T f$array_merge(const T &a1);

template<class T>
T f$array_merge(const T &a1, const T &a2);

template<class T>
T f$array_merge(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(),
                const T &a7 = T(), const T &a8 = T(), const T &a9 = T(),
                const T &a10 = T(), const T &a11 = T(), const T &a12 = T());

template<class T, class T1>
void f$array_merge_into(T &a, const T1 &another_array);

template<class T>
T f$array_replace(const T &base_array, const T &replacements = T());

template<class T>
T f$array_replace(const T &base_array,
  const T &replacements_1, const T &replacements_2, const T &replacements_3 = T(),
  const T &replacements_4 = T(), const T &replacements_5 = T(), const T &replacements_6 = T(),
  const T &replacements_7 = T(), const T &replacements_8 = T(), const T &replacements_9 = T(),
  const T &replacements_10 = T(), const T &replacements_11 = T());

template<class T, class T1>
array<T> f$array_intersect_key(const array<T> &a1, const array<T1> &a2);

template<class T, class T1>
array<T> f$array_intersect(const array<T> &a1, const array<T1> &a2);

template<class T, class T1>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2);

template<class T, class T1, class T2>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3);

template<class T, class T1>
array<T> f$array_diff_key(const array<T> &a1, const array<T1> &a2);

template<class T, class T1>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2);

template<class T, class T1, class T2>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2, const array<T2> &a3);

template<class T, class T1>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2);

template<class T, class T1, class T2>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3);

template<class T>
array<T> f$array_reverse(const array<T> &a, bool preserve_keys = false);

template<class T>
T f$array_shift(array<T> &a);

template<class T, class T1>
int64_t f$array_unshift(array<T> &a, const T1 &val);


template<class T>
bool f$array_key_exists(int64_t int_key, const array<T> &a);

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a);

template<class T>
bool f$array_key_exists(const mixed &v, const array<T> &a);

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a);

template<class K, class T, class = vk::enable_if_in_list<K, vk::list_of_types<double, bool>>>
bool f$array_key_exists(K, const array<T> &);

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict = false);

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a);

template<class T>
mixed f$array_rand(const array<T> &a, int64_t num);


template<class T>
array<typename array<T>::key_type> f$array_keys(const array<T> &a);

template<class T>
array<string> f$array_keys_as_strings(const array<T> &a);

template<class T>
array<int64_t> f$array_keys_as_ints(const array<T> &a);

template<class T>
array<T> f$array_values(const array<T> &a);

template<class T>
array<T> f$array_unique(const array<T> &a, int64_t flags = SORT_STRING);

template<class T>
array<int64_t> f$array_count_values(const array<T> &a);

template<class T>
array<typename array<T>::key_type> f$array_flip(const array<T> &a);

template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict = false);


template<class T>
array<T> f$array_fill(int64_t start_index, int64_t num, const T &value);

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &keys, const T &value);

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values);

template<class T1, class T2>
int64_t f$array_push(array<T1> &a, const T2 &val);

template<class T1, class T2, class T3>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3);

template<class T1, class T2, class T3, class T4>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4);

template<class T1, class T2, class T3, class T4, class T5>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5);

template<class T1, class T2, class T3, class T4, class T5, class T6>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6);

template<class T>
T f$array_pop(array<T> &a);

template<class T>
void f$array_reserve(array<T> &a, int64_t int_size, int64_t string_size, bool make_vector_if_possible = true);

template<class T>
void f$array_reserve_vector(array<T> &a, int64_t size);

template<class T>
void f$array_reserve_map_int_keys(array<T> &a, int64_t size);

template<class T>
void f$array_reserve_map_string_keys(array<T> &a, int64_t size);

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base);

template<class T>
bool f$array_is_vector(const array<T> &a);

template<class T>
bool f$array_is_list(const array<T> &a);

array<mixed> f$range(const mixed &from, const mixed &to, int64_t step = 1);


template<class T>
void f$shuffle(array<T> &a);

template<class T>
void f$sort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T>
void f$rsort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T, class T1>
void f$usort(array<T> &a, const T1 &compare);

template<class T>
void f$asort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T>
void f$arsort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T, class T1>
void f$uasort(array<T> &a, const T1 &compare);

template<class T>
void f$ksort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T>
void f$krsort(array<T> &a, int64_t flag = SORT_REGULAR);

template<class T, class T1>
void f$uksort(array<T> &a, const T1 &compare);

template<class T>
void f$natsort(array<T> &a);

template<class T, class ReturnT = std::conditional_t<std::is_same<T, int64_t>{}, int64_t, double>>
ReturnT f$array_sum(const array<T> &a);


template<class T>
mixed f$getKeyByPos(const array<T> &a, int64_t pos);

template<class T>
T f$getValueByPos(const array<T> &a, int64_t pos);

template<class T>
inline array<T> f$create_vector(int64_t n, const T &default_value);

template<class T>
mixed f$array_first_key(const array<T> &a);

template<class T>
T f$array_first_value(const array<T> &a);

template<class T>
mixed f$array_last_key(const array<T> &a);

template<class T>
T f$array_last_value(const array<T> &a);

template<class T>
inline void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept;


/*
 *
 *     IMPLEMENTATION
 *
 */

string implode_string_vector(const string &s, const array<string> &a);

template<class T>
string f$implode(const string &s, const array<T> &a) {
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
      return implode_string_vector(s, a);
    }
  }

  // fallback to the generic iterator + string_buffer solution

  string_buffer &SB = static_SB;
  SB.clean();

  auto it = a.begin(), it_end = a.end();
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


template<class T>
array<array<T>> f$array_chunk(const array<T> &a, int64_t chunk_size, bool preserve_keys) {
  if (unlikely(chunk_size <= 0)) {
    php_warning("Parameter chunk_size if function array_chunk must be positive");
    return array<array<T>>();
  }

  array<array<T>> result(array_size(a.count() / chunk_size + 1, true));

  array_size new_size = a.size().cut(chunk_size);
  if (!preserve_keys) {
    new_size.size = min(chunk_size, a.count());
    new_size.is_vector = true;
  }

  array<T> res(new_size);
  for (const auto &it : a) {
    if (res.count() == chunk_size) {
      result.emplace_back(std::move(res));
      res = array<T>(new_size);
    }

    if (preserve_keys) {
      res.set_value(it);
    } else {
      res.push_back(it.get_value());
    }
  }

  if (res.count()) {
    result.emplace_back(std::move(res));
  }

  return result;
}

template<class T>
array<T> f$array_slice(const array<T> &a, int64_t offset, const mixed &length_var, bool preserve_keys) {
  auto size = a.count();

  int64_t length = 0;
  if (length_var.is_null()) {
    length = size;
  } else {
    length = length_var.to_int();
  }

  if (offset < 0) {
    offset += size;

    if (offset < 0) {
      offset = 0;
    }
  } else if (offset > size) {
    offset = size;
  }

  if (length < 0) {
    length = size - offset + length;
  }
  if (length <= 0) {
    return array<T>();
  }
  if (size - offset < length) {
    length = size - offset;
  }

  array_size result_size = a.size().cut(length);
  result_size.is_vector = (!preserve_keys && a.has_no_string_keys()) || (preserve_keys && offset == 0 && a.is_vector());

  array<T> result(result_size);
  auto it = a.middle(offset);
  while (length-- > 0) {
    if (preserve_keys) {
      result.set_value(it);
    } else {
      result.push_back(it);
    }
    ++it;
  }

  return result;
}

template<class T>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length, const array<Unknown> &) {
  return f$array_splice(a, offset, length, array<T>());
}

template<class T, class T1>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length, const array<T1> &replacement) {
  int64_t size = a.count();
  if (offset < 0) {
    offset += size;

    if (offset < 0) {
      offset = 0;
    }
  } else if (offset > size) {
    offset = size;
  }

  if (length < 0) {
    length = size - offset + length;

    if (length <= 0) {
      length = 0;
    }
  }
  if (size - offset < length) {
    length = size - offset;
  }

  if (offset == size) {
    a.merge_with(replacement);
    return array<T>();
  }

  array<T> result(a.size().cut(length));
  array<T> new_a(a.size().cut(size - length) + replacement.size());
  int64_t i = 0;
  const auto &const_arr = a;
  for (const auto &it : const_arr) {
    if (i == offset) {
      for (const auto &it_r : replacement) {
        new_a.push_back(it_r.get_value());
      }
    }
    if (i < offset || i >= offset + length) {
      new_a.push_back(it);
    } else {
      result.push_back(it);
    }
    ++i;
  }
  a = std::move(new_a);

  return result;
}

template<class ReturnT, class InputArrayT, class DefaultValueT>
ReturnT f$array_pad(const array<InputArrayT> &a, int64_t size, const DefaultValueT &default_value) {
  auto mod_size = static_cast<size_t>(std::abs(size));

  if (mod_size <= static_cast<size_t>(a.count())) {
    return a;
  }

  constexpr static size_t max_size = 1 << 20;
  if (unlikely(mod_size >= 1 << 20)) {
    php_warning("You may only pad up to 1048576 elements at a time: %zu", max_size);
    return {};
  }

  int64_t new_index = 0;
  ReturnT result_array;

  auto copy_input_to_result = [&] {
    for (const auto &it : a) {
      mixed key = it.get_key();
      const auto &value = it.get_value();

      if (key.is_int()) {
        result_array.set_value(new_index, value);
        new_index++;
      } else {
        result_array.set_value(key, value);
      }
    }
  };

  auto fill_with_default_value = [&] {
    for (size_t i = 0; i < mod_size - a.count(); ++i) {
      result_array.set_value(new_index, default_value);
      new_index++;
    }
  };

  if (size > 0) {
    copy_input_to_result();
    fill_with_default_value();
  } else {
    fill_with_default_value();
    copy_input_to_result();
  }

  return result_array;
}

template<class ReturnT, class DefaultValueT>
ReturnT f$array_pad(const array<Unknown> &, int64_t size, const DefaultValueT &default_value) {
  if (size == 0) {
    return {};
  }

  return f$array_fill(0, std::abs(size), default_value);
}

template<class T>
inline void extract_array_column(array<T> &dest, const array<T> &source, const mixed &column_key, const mixed &index_key) {
  static_assert(!is_class_instance<T>{}, "using index_key is prohibited with array of class_instances");
  if (!source.has_key(column_key)) {
    return;
  }

  auto column = source.get_value(column_key);
  if (!index_key.is_null() && source.has_key(index_key)) {
    auto index = source.get_var(index_key);
    if (index.is_bool() || index.is_numeric()) {
      index.convert_to_int();
    } else if (index.is_null()) {
      php_warning("index_key is null behaviour depends on php version");
      index = string{};
    }

    if (!index.is_array()) {
      dest.set_value(index, column);
      return;
    }
  }

  dest.push_back(column);
}

template<class T>
void extract_array_column_instance(array<class_instance<T>> &dest, const array<class_instance<T>> &source, const mixed &column_key, const mixed &) {
  if (source.has_key(column_key)) {
    dest.push_back(source.get_value(column_key));
  }
}

template<class T, class FunT, class ResT = vk::decay_function_arg_t<FunT, 0>>
Optional<ResT> array_column_helper(const array<T> &a, mixed column_key, mixed index_key, FunT &&element_transformer) {
  ResT result;

  if (unlikely(!column_key.is_string() && !column_key.is_numeric())) {
    php_warning("Parameter column_key must be string or number");
    return false;
  }

  if (unlikely(!index_key.is_null() && !index_key.is_string() && !index_key.is_numeric())) {
    php_warning("Parameter index_key must be string or number or null");
    return false;
  }

  if (column_key.is_float()) {
    column_key.convert_to_int();
  }

  if (index_key.is_float()) {
    index_key.convert_to_int();
  }

  for (const auto &it : a) {
    element_transformer(result, it.get_value(), column_key, index_key);
  }

  return result;
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<array<class_instance<T>>> &a, const mixed &column_key) {
  return array_column_helper(a, column_key, {}, extract_array_column_instance<T>);
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<Optional<array<class_instance<T>>>> &a, const mixed &column_key) {
  auto element_transformer = [] (array<class_instance<T>> &dest, const Optional<array<class_instance<T>>> &source, const mixed &column_key, const mixed &index_key) {
    if (source.has_value()) {
      extract_array_column_instance(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, {}, std::move(element_transformer));
}

template<class T>
Optional<array<T>> f$array_column(const array<array<T>> &a, const mixed &column_key, const mixed &index_key = {}) {
  return array_column_helper(a, column_key, index_key, extract_array_column<T>);

}

template<class T>
Optional<array<T>> f$array_column(const array<Optional<array<T>>> &a, const mixed &column_key, const mixed &index_key = {}) {
  auto element_transformer = [] (array<T> &dest, const Optional<array<T>> &source, const mixed &column_key, const mixed &index_key) {
    if (source.has_value()) {
      extract_array_column(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

inline Optional<array<mixed>> f$array_column(const array<mixed> &a, const mixed &column_key, const mixed &index_key = {}) {
  auto element_transformer = [] (array<mixed> &dest, const mixed &source, const mixed &column_key, const mixed &index_key) {
    if (source.is_array()) {
      extract_array_column(dest, source.as_array(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

template<class T>
inline auto f$array_column(const Optional<T> &a, const mixed &column_key, const mixed &index_key = {}) -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
  if (!a.has_value()) {
    php_warning("first parameter of array_column must be array");
    return false;
  }

  return f$array_column(a.val(), column_key, index_key);
}

template<class T, class F>
array<T> array_filter_impl(const array<T> &a, const F &pred) noexcept {
  array<T> result(a.size());
  for (const auto &it : a) {
    if (pred(it)) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T>
array<T> f$array_filter(const array<T> &a) noexcept {
  return array_filter_impl(a, [](const auto &it) {
    return f$boolval(it.get_value());
  });
}

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback) noexcept {
  return array_filter_impl(a, [&callback](const auto &it) {
    return f$boolval(callback(it.get_value()));
  });
}

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T> &a, const T1 &callback) noexcept {
  return array_filter_impl(a, [&callback](const auto &it) {
    return f$boolval(callback(it.get_key()));
  });
}


template<class T, class CallbackT, class R = typename std::invoke_result_t<std::decay_t<CallbackT>, T>>
array<R> f$array_map(const CallbackT &callback, const array<T> &a) {
  array<R> result(a.size());
  for (const auto &it : a) {
    result.set_value(it.get_key(), callback(it.get_value()));
  }

  return result;
}

template<class R, class T, class CallbackT, class InitialT>
R f$array_reduce(const array<T> &a, const CallbackT &callback, InitialT initial) {
  R result(std::move(initial));
  for (const auto &it : a) {
    result = callback(result, it.get_value());
  }

  return result;
}

template<class T>
T f$array_merge_spread(const T &a1) {
  if (!a1.is_vector()) {
    php_warning("Cannot unpack array with string keys");
  }
  return f$array_merge(a1);
}

template<class T>
T f$array_merge_spread(const T &a1, const T &a2) {
  if (!a1.is_vector() || !a2.is_vector()) {
    php_warning("Cannot unpack array with string keys");
  }
  return f$array_merge(a1, a2);
}

template<class T>
T f$array_merge_spread(const T &a1, const T &a2, const T &a3, const T &a4, const T &a5, const T &a6,
                const T &a7, const T &a8, const T &a9,
                const T &a10, const T &a11, const T &a12) {
  if (!a1.is_vector() || !a2.is_vector() || !a3.is_vector() || !a4.is_vector() ||
      !a5.is_vector() || !a6.is_vector() || !a7.is_vector() || !a8.is_vector() ||
      !a9.is_vector() || !a10.is_vector() || !a11.is_vector() || !a12.is_vector()) {
    php_warning("Cannot unpack array with string keys");
  }
  return f$array_merge(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

template<class T>
T f$array_merge(const T &a1) {
  T result(a1.size());
  result.merge_with(a1);
  return result;
}

template<class T>
T f$array_merge(const T &a1, const T &a2) {
  T result(a1.size() + a2.size());
  result.merge_with(a1);
  result.merge_with(a2);
  return result;
}

template<class ReturnT, class ...Args>
ReturnT f$array_merge_recursive(const Args &...args) {
  array<mixed> result{(args.size() + ... + array_size{})};
  (result.merge_with_recursive(args), ...);
  return result;
}

template<class T, class T1>
void f$array_merge_into(T &a, const T1 &another_array) {
  a.merge_with(another_array);
}

template<class T>
T f$array_merge(const T &a1, const T &a2, const T &a3, const T &a4, const T &a5, const T &a6,
                const T &a7, const T &a8, const T &a9,
                const T &a10, const T &a11, const T &a12) {
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

template<class T>
T f$array_replace(const T &base_array, const T &replacements) {
  auto result = T::convert_from(base_array);
  for (const auto &it : replacements) {
    result.set_value(it);
  }
  return result;
}

template<class T>
T f$array_replace(const T &base_array,
                  const T &replacements_1, const T &replacements_2, const T &replacements_3,
                  const T &replacements_4, const T &replacements_5, const T &replacements_6,
                  const T &replacements_7, const T &replacements_8, const T &replacements_9,
                  const T &replacements_10, const T &replacements_11) {
  return f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(f$array_replace(
           base_array, replacements_1), replacements_2), replacements_3), replacements_4), replacements_5), replacements_6),
           replacements_7), replacements_8),replacements_9), replacements_10), replacements_11);
}

template<class T, class T1>
array<T> f$array_intersect_key(const array<T> &a1, const array<T1> &a2) {
  array<T> result(a1.size().min(a2.size()));
  for (const auto &it : a1) {
    if (a2.has_key(it.get_key())) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_intersect(const array<T> &a1, const array<T1> &a2) {
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
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2) {
  array<T> result(a1.size().min(a2.size()));

  if (!a2.empty()) {
    for (const auto &it : a1) {
      auto key1 = it.get_key();
      if (a2.has_key(key1) && f$strval(a2.get_var(key1)) == f$strval(it.get_value())) {
        result.set_value(it);
      }
    }
  }

  return result;
}

template<class T, class T1, class T2>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  return f$array_intersect_assoc(f$array_intersect_assoc(a1, a2), a3);
}

template<class T, class T1>
array<T> f$array_diff_key(const array<T> &a1, const array<T1> &a2) {
  array<T> result(a1.size());
  for (const auto &it : a1) {
    if (!a2.has_key(it.get_key())) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1, class Proj>
array<T> array_diff_impl(const array<T> &a1, const array<T1> &a2, const Proj &projector) {
  array<T> result(a1.size());

  array<int64_t> values{array_size{a2.count(), false}};

  for (const auto &it : a2) {
    values.set_value(projector(it.get_value()), 1);
  }

  for (const auto &it : a1) {
    if (!values.has_key(projector(it.get_value()))) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2) {
  return array_diff_impl(a1, a2, [](const auto &val) { return f$strval(val); });
}

template<>
array<int64_t> f$array_diff(const array<int64_t> &a1, const array<int64_t> &a2);

template<class T, class T1, class T2>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  return f$array_diff(f$array_diff(a1, a2), a3);
}

template<class T, class T1>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2) {
  array<T> result;
  if (a2.empty()) {
    result = a1;
  } else {
    result = array<T>(a1.size());
    for (const auto &it : a1) {
      auto key1 = it.get_key();
      if (!a2.has_key(key1) || f$strval(a2.get_var(key1)) != f$strval(it.get_value())) {
        result.set_value(it);
      }
    }
  }
  return result;
}

template<class T, class T1, class T2>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  return f$array_diff_assoc(f$array_diff_assoc(a1, a2), a3);
}

template<class T>
array<T> f$array_reverse(const array<T> &a, bool preserve_keys) {
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


template<class T>
T f$array_shift(array<T> &a) {
  return a.shift();
}

template<class T, class T1>
int64_t f$array_unshift(array<T> &a, const T1 &val) {
  return a.unshift(val);
}


template<class T>
bool f$array_key_exists(int64_t int_key, const array<T> &a) {
  return a.has_key(int_key);
}

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a) {
  return a.has_key(string_key);
}

template<class T>
bool f$array_key_exists(const mixed &v, const array<T> &a) {
  return (v.is_int() || v.is_string() || v.is_null()) && a.has_key(v);
}

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a) {
  return f$array_key_exists(mixed(v), a);
}

template<class K, class T, class>
bool f$array_key_exists(K, const array<T> &) {
  return false;
}

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict) {
  for (const auto &it : a) {
    if (strict ? equals(it.get_value(), val) : eq2(it.get_value(), val)) {
      return it.get_key();
    }
  }

  return typename array<T>::key_type(false);
}

template<class T, class T1>
std::tuple<typename array<T>::key_type, T> f$array_find(const array<T> &a,  const T1 &callback) {
  for (const auto &it : a) {
    if (callback(it.get_value())) {
      return std::make_tuple(it.get_key(), it.get_value());
    }
  }

  return {};
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a) {
  if (int64_t size = a.count()) {
    return a.middle(f$mt_rand(0, size - 1)).get_key();
  }
  return {};
}

template<class T>
mixed f$array_rand(const array<T> &a, int64_t num) {
  if (num == 1) {
    return f$array_rand(a);
  }

  int64_t size = a.count();

  if (unlikely(num <= 0 || num > size)) {
    php_warning("Second argument has to be between 1 and the number of elements in the array");
    return {};
  }

  array<typename array<T>::key_type> result(array_size(num, true));
  for (const auto &it : a) {
    if (f$mt_rand(0, --size) < num) {
      result.push_back(it.get_key());
      --num;
    }
  }

  return result;
}

template<class T, class F>
auto transform_to_vector(const array<T> &a, const F &op) {
  array<typename vk::function_traits<F>::ResultType> result(array_size(a.count(), true));
  for (const auto &it : a) {
    result.push_back(op(it));
  }
  return result;
}

template<class T>
array<typename array<T>::key_type> f$array_keys(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_key(); });
}

template<class T>
array<string> f$array_keys_as_strings(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_key().to_string(); });
}

template<class T>
array<int64_t> f$array_keys_as_ints(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_key().to_int(); });
}

template<class T>
array<T> f$array_values(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_value(); });
}

template<class T>
array<T> f$array_unique(const array<T> &a, int64_t flags) {
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
array<int64_t> f$array_count_values(const array<T> &a) {
  array<int64_t> result(array_size(a.count(), false));

  for (const auto &it : a) {
    ++result[f$strval(it.get_value())];
  }
  return result;
}

template<class T>
array<typename array<T>::key_type> f$array_flip(const array<T> &a) {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");
  array<typename array<T>::key_type> result;

  for (const auto &it : a) {
    const auto &value = it.get_value();
    if (vk::is_type_in_list<T, int64_t, string>{} || f$is_int(value) || f$is_string(value)) {
      result.set_value(value, it.get_key());
    } else {
      php_warning("Unsupported type of array element \"%s\" in function array_flip", get_type_c_str(value));
    }
  }

  return result;
}

template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict) {
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


template<class T>
array<T> f$array_fill(int64_t start_index, int64_t num, const T &value) {
  if (unlikely(num < 0)) {
    php_warning("Parameter num of array_fill must not be negative");
    return {};
  }
  if (num == 0) {
    return {};
  }
  array<T> result(array_size(num, start_index == 0));

  if (result.is_vector()) {
    result.fill_vector(num, value);
  } else {
    result.set_value(start_index, value);
    while (--num > 0) {
      result.push_back(value);
    }
  }

  return result;
}

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &keys, const T &value) {
  static_assert(!std::is_same<T1, int>{}, "int is forbidden");
  array<T> result{array_size{keys.count(), false}};
  for (const auto &it : keys) {
    const auto &key = it.get_value();
    if (vk::is_type_in_list<T1, string, int64_t>{} || f$is_int(key)) {
      result.set_value(key, value);
    } else {
      result.set_value(f$strval(key), value);
    }
  }

  return result;
}

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values) {
  if (unlikely(keys.count() != values.count())) {
    php_warning("Size of arrays keys and values must be the same in function array_combine");
    return {};
  }

  static_assert(!std::is_same<T1, int>{}, "int is forbidden");
  array<T> result{array_size{keys.count(), false}};
  auto it_values = values.begin();
  auto it_keys = keys.begin();
  const auto it_keys_last = keys.end();
  for (; it_keys != it_keys_last; ++it_keys, ++it_values) {
    const auto &key = it_keys.get_value();
    if (vk::is_type_in_list<T1, string, int64_t>{} || f$is_int(key)) {
      result.set_value(key, it_values.get_value());
    } else {
      result.set_value(f$strval(key), it_values.get_value());
    }
  }

  return result;
}

template<class T1, class T2>
int64_t f$array_push(array<T1> &a, const T2 &val) {
  a.push_back(val);
  return a.count();
}

template<class T1, class T2, class T3>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3) {
  a.push_back(val2);
  a.push_back(val3);
  return a.count();
}

template<class T1, class T2, class T3, class T4>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4) {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  return a.count();
}


template<class T1, class T2, class T3, class T4, class T5>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5) {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  a.push_back(val5);
  return a.count();
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6) {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  a.push_back(val5);
  a.push_back(val6);
  return a.count();
}


template<class T>
T f$array_pop(array<T> &a) {
  return a.pop();
}

template<class T>
T f$array_unset(array<T> &arr, int64_t key) {
  return arr.unset(key);
}

template<class T>
T f$array_unset(array<T> &arr, const string &key) {
  return arr.unset(key);
}

template<class T>
T f$array_unset(array<T> &arr, const mixed &key) {
  return arr.unset(key);
}

template<class T>
void f$array_reserve(array<T> &a, int64_t int_size, int64_t string_size, bool make_vector_if_possible) {
  a.reserve(int_size + string_size, make_vector_if_possible);
}

template<class T>
void f$array_reserve_vector(array<T> &a, int64_t size) {
  a.reserve(size, true);
}

template<class T>
void f$array_reserve_map_int_keys(array<T> &a, int64_t size) {
  a.reserve(size, false);
}

template<class T>
void f$array_reserve_map_string_keys(array<T> &a, int64_t size) {
  a.reserve(size, false);
}

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base) {
  auto size_info = base.size();
  f$array_reserve(a, size_info.size, size_info.is_vector);
}

template<class T>
bool f$array_is_vector(const array<T> &a) {
  return a.is_vector();
}

template<class T>
bool f$array_is_list(const array<T> &a) {
  // is_vector() is fast, but not enough;
  // is_pseudo_vector() doesn't cover is_vector() case,
  // so we need to call both of them to get the precise and PHP-compatible answer
  return a.is_vector() || a.is_pseudo_vector();
}


template<class T>
void f$shuffle(array<T> &a) {
  int64_t n = a.count();
  if (n <= 1) {
    return;
  }

  array<T> result(array_size(n, true));
  const auto &const_arr = a;
  for (const auto &it : const_arr) {
    result.push_back(it.get_value());
  }

  for (int64_t i = 1; i < n; i++) {
    swap(result[i], result[f$mt_rand(0, i)]);
  }

  a = std::move(result);
}

template<class T>
struct sort_compare {
  bool operator()(const T &h1, const T &h2) const {
    return lt(h2, h1);
  }
};

template<class T>
struct sort_compare_numeric {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  bool operator()(const T &h1, const T &h2) const {
    return f$floatval(h1) > f$floatval(h2);
  }
};

template<>
struct sort_compare_numeric<int64_t> : std::greater<int64_t> {
};

template<class T>
struct sort_compare_string {
  bool operator()(const T &h1, const T &h2) const {
    return f$strval(h1).compare(f$strval(h2)) > 0;
  }
};

template<class T>
struct sort_compare_natural {
  bool operator()(const T &h1, const T &h2) const {
    return f$strnatcmp(f$strval(h1), f$strval(h2)) > 0;
  }
};

template<class T>
void f$sort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.sort(sort_compare<T>(), true);
    case SORT_NUMERIC:
      return a.sort(sort_compare_numeric<T>(), true);
    case SORT_STRING:
      return a.sort(sort_compare_string<T>(), true);
    default:
      php_warning("Unsupported sort_flag in function sort");
  }
}


template<class T>
struct rsort_compare {
  bool operator()(const T &h1, const T &h2) const {
    return lt(h1, h2);
  }
};

template<class T>
struct rsort_compare_numeric {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  bool operator()(const T &h1, const T &h2) const {
    return f$floatval(h1) < f$floatval(h2);
  }
};

template<>
struct rsort_compare_numeric<int64_t> : std::less<int64_t> {
};

template<class T>
struct rsort_compare_string {
  bool operator()(const T &h1, const T &h2) const {
    return f$strval(h1).compare(f$strval(h2)) < 0;
  }
};

template<class T>
void f$rsort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.sort(rsort_compare<T>(), true);
    case SORT_NUMERIC:
      return a.sort(rsort_compare_numeric<T>(), true);
    case SORT_STRING:
      return a.sort(rsort_compare_string<T>(), true);
    default:
      php_warning("Unsupported sort_flag in function rsort");
  }
}


template<class T, class T1>
void f$usort(array<T> &a, const T1 &compare) {
  return a.sort(compare, true);
}


template<class T>
void f$asort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.sort(sort_compare<T>(), false);
    case SORT_NUMERIC:
      return a.sort(sort_compare_numeric<T>(), false);
    case SORT_STRING:
      return a.sort(sort_compare_string<T>(), false);
    default:
      php_warning("Unsupported sort_flag in function asort");
  }
}

template<class T>
void f$arsort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.sort(rsort_compare<T>(), false);
    case SORT_NUMERIC:
      return a.sort(rsort_compare_numeric<T>(), false);
    case SORT_STRING:
      return a.sort(rsort_compare_string<T>(), false);
    default:
      php_warning("Unsupported sort_flag in function arsort");
  }
}

template<class T, class T1>
void f$uasort(array<T> &a, const T1 &compare) {
  return a.sort(compare, false);
}


template<class T>
void f$ksort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.ksort(sort_compare<typename array<T>::key_type>());
    case SORT_NUMERIC:
      return a.ksort(sort_compare_numeric<typename array<T>::key_type>());
    case SORT_STRING:
      return a.ksort(sort_compare_string<typename array<T>::key_type>());
    default:
      php_warning("Unsupported sort_flag in function ksort");
  }
}

template<class T>
void f$krsort(array<T> &a, int64_t flag) {
  switch (flag) {
    case SORT_REGULAR:
      return a.ksort(rsort_compare<typename array<T>::key_type>());
    case SORT_NUMERIC:
      return a.ksort(rsort_compare_numeric<typename array<T>::key_type>());
    case SORT_STRING:
      return a.ksort(rsort_compare_string<typename array<T>::key_type>());
    default:
      php_warning("Unsupported sort_flag in function krsort");
  }
}

template<class T, class T1>
void f$uksort(array<T> &a, const T1 &compare) {
  return a.ksort(compare);
}

template<class T>
void f$natsort(array<T> &a) {
  return a.sort(sort_compare_natural<typename array<T>::key_type>(), false);
}

template<class T, class ReturnT>
ReturnT f$array_sum(const array<T> &a) {
  static_assert(!std::is_same_v<T, int>, "int is forbidden");

  ReturnT result = 0;
  for (const auto &it : a) {
    if constexpr (std::is_same_v<T, int64_t>) {
      result += it.get_value();
    } else {
      result += f$floatval(it.get_value());
    }
  }
  return result;
}


template<class T>
mixed f$getKeyByPos(const array<T> &a, int64_t pos) {
  auto it = a.middle(pos);
  return it == a.end() ? mixed{} : it.get_key();
}

template<class T>
T f$getValueByPos(const array<T> &a, int64_t pos) {
  auto it = a.middle(pos);
  return it == a.end() ? T{} : it.get_value();
}

template<class T>
array<T> f$create_vector(int64_t n, const T &default_value) {
  array<T> res(array_size(n, true));
  for (int64_t i = 0; i < n; i++) {
    res.push_back(default_value);
  }
  return res;
}

template<class T>
mixed f$array_first_key(const array<T> &a) {
  return a.empty() ? mixed() : a.begin().get_key();
}

template<class T>
mixed f$array_key_first(const array<T> &a) {
  return f$array_first_key(a);
}

template<class T>
T f$array_first_value(const array<T> &a) {
  return a.empty() ? T() : a.begin().get_value(); // in PHP 'false' on empty, here T()
}

template<class T>
mixed f$array_last_key(const array<T> &a) {
  return a.empty() ? mixed() : (--a.end()).get_key();
}

template<class T>
mixed f$array_key_last(const array<T> &a) {
  return f$array_last_key(a);
}

template<class T>
T f$array_last_value(const array<T> &a) {
  return a.empty() ? T() : (--a.end()).get_value(); // in PHP 'false' on empty, here T()
}

template<class T>
void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept {
  a.swap_int_keys(idx1, idx2);
}


template<class T>
T vk_dot_product_sparse(const array<T> &a, const array<T> &b) {
  T result = T();
  for (const auto &it : a) {
    const auto *b_val = b.find_value(it);
    if (b_val && !f$is_null(*b_val)) {
      result += it.get_value() * (*b_val);
    }
  }
  return result;
}

template<class T>
T vk_dot_product_dense(const array<T> &a, const array<T> &b) {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  T result = T();
  int64_t size = min(a.count(), b.count());
  for (int64_t i = 0; i < size; i++) {
    result += a.get_value(i) * b.get_value(i);
  }
  return result;
}

template<>
inline int64_t vk_dot_product_dense<int64_t>(const array<int64_t> &a, const array<int64_t> &b) {
  const int64_t size = min(a.count(), b.count());
  const int64_t *ap = a.get_const_vector_pointer();
  const int64_t *bp = b.get_const_vector_pointer();
  return std::inner_product(ap, ap + size, bp, 0L);
}


template<>
inline double vk_dot_product_dense<double>(const array<double> &a, const array<double> &b) {
  int64_t size = min(a.count(), b.count());
  const double *ap = a.get_const_vector_pointer();
  const double *bp = b.get_const_vector_pointer();
  return __dot_product(ap, bp, static_cast<int>(size));
}


template<class T>
T f$vk_dot_product(const array<T> &a, const array<T> &b) {
  if (a.is_vector() && b.is_vector()) {
    return vk_dot_product_dense<T>(a, b);
  }
  return vk_dot_product_sparse<T>(a, b);
}
