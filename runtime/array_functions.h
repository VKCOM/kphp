#pragma once

#include <climits>

#include "common/type_traits/function_traits.h"
#include "common/vector-product.h"

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"

template<class T>
string f$implode(const string &s, const array<T> &a);

array<string> explode(char delimiter, const string &str, int limit = INT_MAX);

array<string> f$explode(const string &delimiter, const string &str, int limit = INT_MAX);


template<class T>
array<array<T>> f$array_chunk(const array<T> &a, int chunk_size, bool preserve_keys = false);

template<class T>
array<T> f$array_slice(const array<T> &a, int offset, const var &length_var = var(), bool preserve_keys = false);

template<class T>
array<T> f$array_splice(array<T> &a, int offset, int length = INT_MAX);

template<class T, class T1>
array<T> f$array_splice(array<T> &a, int offset, int length = INT_MAX, const array<T1> &replacement = array<var>());

template<class ReturnT, class InputArrayT, class DefaultValueT>
ReturnT f$array_pad(const array<InputArrayT> &a, int size, const DefaultValueT &default_value);

template<class ReturnT, class DefaultValueT>
ReturnT f$array_pad(const array<Unknown> &a, int size, const DefaultValueT &default_value);

template<class T>
array<T> f$array_filter(const array<T> &a);

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback);

template<class T>
T f$array_merge(const T &a1);

template<class T>
T f$array_merge(const T &a1, const T &a2);

template<class T>
T f$array_merge(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(),
                const T &a7 = T(), const T &a8 = T(), const T &a9 = T(),
                const T &a10 = T(), const T &a11 = T(), const T &a12 = T());

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
int f$array_unshift(array<T> &a, const T1 &val);


template<class T>
bool f$array_key_exists(int int_key, const array<T> &a);

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a);

template<class T>
bool f$array_key_exists(const var &v, const array<T> &a);

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a);

template<class K, class T, class = vk::enable_if_in_list<K, vk::list_of_types<double, bool>>>
bool f$array_key_exists(K, const array<T> &);

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict = false);

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a);

template<class T>
var f$array_rand(const array<T> &a, int num);


template<class T>
array<typename array<T>::key_type> f$array_keys(const array<T> &a);

template<class T>
array<string> f$array_keys_as_strings(const array<T> &a);

template<class T>
array<int> f$array_keys_as_ints(const array<T> &a);

template<class T>
array<T> f$array_values(const array<T> &a);

template<class T>
array<T> f$array_unique(const array<T> &a);

template<class T>
array<int> f$array_count_values(const array<T> &a);

array<array<var>::key_type> f$array_flip(const array<var> &a);

array<array<var>::key_type> f$array_flip(const array<int> &a);

array<array<var>::key_type> f$array_flip(const array<string> &a);

template<class T>
array<typename array<T>::key_type> f$array_flip(const array<T> &a);

template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict = false);


template<class T>
array<T> f$array_fill(int start_index, int num, const T &value);

template<class T>
array<T> f$array_fill_keys(const array<int> &a, const T &value);

template<class T>
array<T> f$array_fill_keys(const array<string> &a, const T &value);

template<class T>
array<T> f$array_fill_keys(const array<var> &a, const T &value);

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &a, const T &value);

template<class T>
array<T> f$array_combine(const array<int> &keys, const array<T> &values);

template<class T>
array<T> f$array_combine(const array<string> &keys, const array<T> &values);

template<class T>
array<T> f$array_combine(const array<var> &keys, const array<T> &values);

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values);

template<class T1, class T2>
int f$array_push(array<T1> &a, const T2 &val);

template<class T1, class T2, class T3>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3);

template<class T1, class T2, class T3, class T4>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4);

template<class T1, class T2, class T3, class T4, class T5>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5);

template<class T1, class T2, class T3, class T4, class T5, class T6>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6);

template<class T>
T f$array_pop(array<T> &a);

template<class T>
void f$array_reserve(array<T> &a, int int_size, int string_size, bool make_vector_if_possible = true);

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base);

template<class T>
bool f$array_is_vector(array<T> &a);


array<var> f$range(const var &from, const var &to, int step = 1);


template<class T>
void f$shuffle(array<T> &a);

const int SORT_REGULAR = 0;
const int SORT_NUMERIC = 1;
const int SORT_STRING = 2;

template<class T>
void f$sort(array<T> &a, int flag = SORT_REGULAR);

template<class T>
void f$rsort(array<T> &a, int flag = SORT_REGULAR);

template<class T, class T1>
void f$usort(array<T> &a, const T1 &compare);

template<class T>
void f$asort(array<T> &a, int flag = SORT_REGULAR);

template<class T>
void f$arsort(array<T> &a, int flag = SORT_REGULAR);

template<class T, class T1>
void f$uasort(array<T> &a, const T1 &compare);

template<class T>
void f$ksort(array<T> &a, int flag = SORT_REGULAR);

template<class T>
void f$krsort(array<T> &a, int flag = SORT_REGULAR);

template<class T, class T1>
void f$uksort(array<T> &a, const T1 &compare);

template<class T>
void f$natsort(array<T> &a);

int f$array_sum(const array<int> &a);

double f$array_sum(const array<double> &a);

double f$array_sum(const array<var> &a);

template<class T>
double f$array_sum(const array<T> &a);


template<class T>
var f$getKeyByPos(const array<T> &a, int pos);

template<class T>
T f$getValueByPos(const array<T> &a, int pos);

template<class T>
inline array<T> f$create_vector(int n, const T &default_value);

inline array<var> f$create_vector(int n);

template<class T>
var f$array_first_key(const array<T> &a);

template<class T>
T f$array_first_value(const array<T> &a);

template<class T>
var f$array_last_key(const array<T> &a);

template<class T>
T f$array_last_value(const array<T> &a);


/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
string f$implode(const string &s, const array<T> &a) {
  string_buffer &SB = static_SB;
  SB.clean();

  typename array<T>::const_iterator it = a.begin(), it_end = a.end();
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
array<array<T>> f$array_chunk(const array<T> &a, int chunk_size, bool preserve_keys) {
  if (chunk_size <= 0) {
    php_warning("Parameter chunk_size if function array_chunk must be positive");
    return array<array<T>>();
  }
  array<array<T>> result(array_size(a.count() / chunk_size + 1, 0, true));

  array_size new_size = a.size().cut(chunk_size);
  if (!preserve_keys) {
    new_size.int_size = min(chunk_size, a.count());
    new_size.string_size = 0;
    new_size.is_vector = true;
  }

  array<T> res(new_size);
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (res.count() == chunk_size) {
      result.push_back(res);
      res = array<T>(new_size);
    }

    if (preserve_keys) {
      res.set_value(it);
    } else {
      res.push_back(it.get_value());
    }
  }

  if (res.count()) {
    result.push_back(res);
  }

  return result;
}

template<class T>
array<T> f$array_slice(const array<T> &a, int offset, const var &length_var, bool preserve_keys) {
  int size = a.count();

  int length;
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
  result_size.is_vector = (!preserve_keys && result_size.string_size == 0) || (preserve_keys && offset == 0 && a.is_vector());

  array<T> result(result_size);
  typename array<T>::const_iterator it = a.middle(offset);
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
array<T> f$array_splice(array<T> &a, int offset, int length) {
  return f$array_splice(a, offset, length, array<T>());
}

template<class T, class T1>
array<T> f$array_splice(array<T> &a, int offset, int length, const array<T1> &replacement) {
  int size = a.count();
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
  int i = 0;
  for (typename array<T>::iterator it = a.begin(); it != a.end(); ++it, i++) {
    if (i == offset) {
      for (typename array<T1>::const_iterator it_r = replacement.begin(); it_r != replacement.end(); ++it_r) {
        new_a.push_back(it_r.get_value());
      }
    }
    if (i < offset || i >= offset + length) {
      new_a.push_back(it);
    } else {
      result.push_back(it);
    }
  }
  a = new_a;

  return result;
}

template<class ReturnT, class InputArrayT, class DefaultValueT>
ReturnT f$array_pad(const array<InputArrayT> &a, int size, const DefaultValueT &default_value) {
  auto mod_size = static_cast<size_t>(std::abs(size));

  if (mod_size <= static_cast<size_t>(a.count())) {
    return a;
  }

  constexpr static size_t max_size = 1 << 20;
  if (mod_size >= 1 << 20) {
    php_warning("You may only pad up to 1048576 elements at a time: %zu", max_size);
    return {};
  }

  int new_index = 0;
  ReturnT result_array;

  auto copy_input_to_result = [&] {
    for (auto it = a.begin(); it != a.end(); ++it) {
      var key = it.get_key();
      auto &value = it.get_value();

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
ReturnT f$array_pad(const array<Unknown> &, int size, const DefaultValueT &default_value) {
  if (size == 0) {
    return {};
  }

  return f$array_fill(0, std::abs(size), default_value);
}

template<class T>
inline void extract_array_column(array<T> &dest, const array<T> &source, const var &column_key, const var &index_key) {
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
void extract_array_column_instance(array<class_instance<T>> &dest, const array<class_instance<T>> &source, const var &column_key, const var &) {
  if (!source.has_key(column_key)) {
    return;
  }
  dest.push_back(source.get_value(column_key));
}

template<class T, class FunT, class ResT = vk::decay_function_arg_t<FunT, 0>>
Optional<ResT> array_column_helper(const array<T> &a, var column_key, var index_key, FunT &&element_transformer) {
  ResT result;

  if (!column_key.is_string() && !column_key.is_numeric()) {
    php_warning("Parameter column_key must be string or number");
    return false;
  }

  if (!index_key.is_null() && !index_key.is_string() && !index_key.is_numeric()) {
    php_warning("Parameter index_key must be string or number or null");
    return false;
  }

  if (column_key.is_float()) {
    column_key.convert_to_int();
  }

  if (index_key.is_float()) {
    index_key.convert_to_int();
  }

  for (auto it = a.begin(); it != a.end(); ++it) {
    element_transformer(result, it.get_value(), column_key, index_key);
  }

  return result;
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<array<class_instance<T>>> &a, const var &column_key) {
  return array_column_helper(a, column_key, {}, extract_array_column_instance<T>);
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<Optional<array<class_instance<T>>>> &a, const var &column_key) {
  auto element_transformer = [] (array<class_instance<T>> &dest, const Optional<array<class_instance<T>>> &source, const var &column_key, const var &index_key) {
    if (source.has_value()) {
      back_inserter_class_instance(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, {}, std::move(element_transformer));
}

template<class T>
Optional<array<T>> f$array_column(const array<array<T>> &a, const var &column_key, const var &index_key = {}) {
  return array_column_helper(a, column_key, index_key, extract_array_column<T>);

}

template<class T>
Optional<array<T>> f$array_column(const array<Optional<array<T>>> &a, const var &column_key, const var &index_key = {}) {
  auto element_transformer = [] (array<T> &dest, const Optional<array<T>> &source, const var &column_key, const var &index_key) {
    if (source.has_value()) {
      extract_array_column(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

inline Optional<array<var>> f$array_column(const array<var> &a, const var &column_key, const var &index_key = {}) {
  auto element_transformer = [] (array<var> &dest, const var &source, const var &column_key, const var &index_key) {
    if (source.is_array()) {
      extract_array_column(dest, source.as_array(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

inline Optional<array<var>> f$array_column(const var &a, const var &column_key, const var &index_key = {}) {
  if (!a.is_array()) {
    php_warning("first parameter of array_column must be array");
    return false;
  }

  return f$array_column(a.as_array(), column_key, index_key);
}

template<class T>
inline auto f$array_column(const Optional<T> &a, const var &column_key, const var &index_key = {}) -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
  if (!a.has_value()) {
    php_warning("first parameter of array_column must be array");
    return false;
  }

  return f$array_column(a.val(), column_key, index_key);
}

template<class T>
array<T> f$array_filter(const array<T> &a) {
  array<T> result(a.size());
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (f$boolval(it.get_value())) {
      result.set_value(it);
    }
  }

  return result;
}

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback) {
  array<T> result(a.size());
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    bool need_set_value;

    need_set_value = f$boolval(callback(it.get_value()));

    if (need_set_value) {
      result.set_value(it);
    }
  }

  return result;
}


template<class T, class CallbackT, class R = typename std::result_of<std::decay_t<CallbackT>(T)>::type>
array<R> f$array_map(const CallbackT &callback, const array<T> &a) {
  array<R> result(a.size());
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    result.set_value(it.get_key(), callback(it.get_value()));
  }

  return result;
}

template<class R, class T, class CallbackT, class InitialT>
R f$array_reduce(const array<T> &a, const CallbackT &callback, const InitialT initial) {
  R result = initial;
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    result = callback(result, it.get_value());
  }

  return result;
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
  for (auto it = replacements.begin(); it != replacements.end(); ++it) {
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
  for (typename array<T>::const_iterator it = a1.begin(); it != a1.end(); ++it) {
    if (a2.has_key(it.get_key())) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_intersect(const array<T> &a1, const array<T1> &a2) {
  array<T> result(a1.size().min(a2.size()));

  array<int> values(array_size(0, a2.count(), false));
  for (typename array<T1>::const_iterator it = a2.begin(); it != a2.end(); ++it) {
    values.set_value(f$strval(it.get_value()), 1);
  }

  for (typename array<T>::const_iterator it = a1.begin(); it != a1.end(); ++it) {
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
    for (auto it = a1.begin(); it != a1.end(); ++it) {
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
  for (typename array<T>::const_iterator it = a1.begin(); it != a1.end(); ++it) {
    if (!a2.has_key(it.get_key())) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2) {
  array<T> result(a1.size());

  array<int> values(array_size(0, a2.count(), false));
  for (typename array<T1>::const_iterator it = a2.begin(); it != a2.end(); ++it) {
    values.set_value(f$strval(it.get_value()), 1);
  }

  for (typename array<T>::const_iterator it = a1.begin(); it != a1.end(); ++it) {
    if (!values.has_key(f$strval(it.get_value()))) {
      result.set_value(it);
    }
  }
  return result;
}

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
    for (auto it = a1.begin(); it != a1.end(); ++it) {
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

  for (typename array<T>::const_iterator it = a.end(); it != a.begin();) {
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
int f$array_unshift(array<T> &a, const T1 &val) {
  return a.unshift(val);
}


template<class T>
bool f$array_key_exists(int int_key, const array<T> &a) {
  return a.has_key(int_key);
}

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a) {
  return a.has_key(string_key);
}

template<class T>
bool f$array_key_exists(const var &v, const array<T> &a) {
  if (!v.is_int() && !v.is_string() && !v.is_null()) {
    return false;
  }
  return a.has_key(v);
}

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a) {
  return f$array_key_exists(var(v), a);
}

template<class K, class T, class>
bool f$array_key_exists(K, const array<T> &) {
  return false;
}

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict) {
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    bool found = strict ? equals(it.get_value(), val) : eq2(it.get_value(), val);
    if (found) {
      return it.get_key();
    }
  }

  return typename array<T>::key_type(false);
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a) {
  int size = a.count();
  if (size == 0) {
    return typename array<T>::key_type();
  }

  return a.middle(rand() % size).get_key();
}

template<class T>
var f$array_rand(const array<T> &a, int num) {
  if (num == 1) {
    return f$array_rand(a);
  }

  int size = a.count();
  if (num > size) {
    num = size;
  }
  if (num <= 0) {
    php_warning("Parameter num of array_rand must be positive");
    return array<typename array<T>::key_type>();
  }

  array<typename array<T>::key_type> result(array_size(num, 0, true));
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (rand() % (size--) < num) {
      result.push_back(it.get_key());
      --num;
    }
  }

  return result;
}

template<class T, class F>
auto transform_to_vector(const array<T> &a, const F &op) {
  array<typename vk::function_traits<F>::ResultType> result(array_size(a.count(), 0, true));
  for (auto it = a.begin(); it != a.end(); ++it) {
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
array<int> f$array_keys_as_ints(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_key().to_int(); });
}

template<class T>
array<T> f$array_values(const array<T> &a) {
  using Iterator = typename array<T>::const_iterator;
  return transform_to_vector(a, [](Iterator it) { return it.get_value(); });
}

template<class T>
array<T> f$array_unique(const array<T> &a) {
  array<int> values(array_size(a.count(), a.count(), false));
  array<T> result(a.size());

  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    const T &value = it.get_value();
    int &cnt = values[f$strval(value)];
    if (!cnt) {
      cnt = 1;
      result.set_value(it);
    }
  }
  return result;
}

template<class T>
array<int> f$array_count_values(const array<T> &a) {
  array<int> result(array_size(0, a.count(), false));

  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    ++result[f$strval(it.get_value())];
  }
  return result;
}


template<class T>
array<typename array<T>::key_type> f$array_flip(const array<T> &a) {//TODO optimize
  return f$array_flip(array<var>(a));
}


template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict) {
  if (!strict) {
    for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
      if (eq2(it.get_value(), value)) {
        return true;
      }
    }
  } else {
    for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
      if (equals(it.get_value(), value)) {
        return true;
      }
    }
  }
  return false;
}


template<class T>
array<T> f$array_fill(int start_index, int num, const T &value) {
  if (num < 0) {
    php_warning("Parameter num of array_fill must not be negative");
    return array<T>();
  }
  if (num == 0) {
    return array<T>();
  }
  array<T> result(array_size(num, 0, start_index == 0));

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

template<class T>
array<T> f$array_fill_keys(const array<int> &a, const T &value) {
  array<T> result(array_size(a.count(), 0, false));
  for (array<int>::const_iterator it = a.begin(); it != a.end(); ++it) {
    result.set_value(it.get_value(), value);
  }

  return result;
}

template<class T>
array<T> f$array_fill_keys(const array<string> &a, const T &value) {
  array<T> result(array_size(0, a.count(), false));
  for (array<string>::const_iterator it = a.begin(); it != a.end(); ++it) {
    result.set_value(it.get_value(), value);
  }

  return result;
}

template<class T>
array<T> f$array_fill_keys(const array<var> &a, const T &value) {
  array<T> result(array_size(a.count(), a.count(), false));
  for (array<var>::const_iterator it = a.begin(); it != a.end(); ++it) {
    const var &key = it.get_value();
    if (!key.is_int()) {
      result.set_value(key.to_string(), value);
    } else {
      result.set_value(key.to_int(), value);
    }
  }

  return result;
}

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &a, const T &value) {
  return f$array_fill_keys(array<var>(a), value);
}


template<class T>
array<T> f$array_combine(const array<int> &keys, const array<T> &values) {
  if (keys.count() != values.count()) {
    php_warning("Size of arrays keys and values must be the same in function array_combine");
    return array<T>();
  }

  array<T> result(array_size(keys.count(), 0, false));
  typename array<T>::const_iterator it_values = values.begin();
  for (array<int>::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys, ++it_values) {
    result.set_value(it_keys.get_value(), it_values.get_value());
  }

  return result;
}

template<class T>
array<T> f$array_combine(const array<string> &keys, const array<T> &values) {
  if (keys.count() != values.count()) {
    php_warning("Size of arrays keys and values must be the same in function array_combine");
    return array<T>();
  }

  array<T> result(array_size(0, keys.count(), false));
  typename array<T>::const_iterator it_values = values.begin();
  for (array<string>::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys, ++it_values) {
    result.set_value(it_keys.get_value(), it_values.get_value());
  }

  return result;
}

template<class T>
array<T> f$array_combine(const array<var> &keys, const array<T> &values) {
  if (keys.count() != values.count()) {
    php_warning("Size of arrays keys and values must be the same in function array_combine");
    return array<T>();
  }

  array<T> result(array_size(keys.count(), keys.count(), false));
  typename array<T>::const_iterator it_values = values.begin();
  for (array<var>::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys, ++it_values) {
    const var &key = it_keys.get_value();
    if (!key.is_int()) {
      result.set_value(key.to_string(), it_values.get_value());
    } else {
      result.set_value(key.to_int(), it_values.get_value());
    }
  }

  return result;
}

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values) {
  return f$array_combine(array<var>(keys), values);
}

template<class T1, class T2>
int f$array_push(array<T1> &a, const T2 &val) {
  a.push_back(val);
  return a.count();
}

template<class T1, class T2, class T3>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3) {
  a.push_back(val2);
  a.push_back(val3);
  return a.count();
}

template<class T1, class T2, class T3, class T4>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4) {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  return a.count();
}


template<class T1, class T2, class T3, class T4, class T5>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5) {
  a.push_back(val2);
  a.push_back(val3);
  a.push_back(val4);
  a.push_back(val5);
  return a.count();
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
int f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6) {
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
void f$array_reserve(array<T> &a, int int_size, int string_size, bool make_vector_if_possible) {
  a.reserve(int_size, string_size, make_vector_if_possible);
}

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base) {
  auto size_info = base.size();
  f$array_reserve(a, size_info.int_size, size_info.string_size, size_info.is_vector);
}

template<class T>
bool f$array_is_vector(array<T> &a) {
  return a.is_vector();
}


template<class T>
void f$shuffle(array<T> &a) {//TODO move into array
  int n = a.count();
  if (n <= 1) {
    return;
  }

  array<T> result(array_size(n, 0, true));
  for (typename array<T>::iterator it = a.begin(); it != a.end(); ++it) {
    result.push_back(it.get_value());
  }

  for (int i = 1; i < n; i++) {
    swap(result[i], result[rand() % (i + 1)]);
  }

  a = result;
}

template<class T>
struct sort_compare {
  bool operator()(const T &h1, const T &h2) const {
    return gt(h1, h2);
  }
};

template<class T>
struct sort_compare_numeric {
  bool operator()(const T &h1, const T &h2) const {
    return f$floatval(h1) > f$floatval(h2);
  }
};

template<>
struct sort_compare_numeric<int> {
  bool operator()(int h1, int h2) const {
    return h1 > h2;
  }
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
void f$sort(array<T> &a, int flag) {
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
  bool operator()(const T &h1, const T &h2) const {
    return f$floatval(h1) < f$floatval(h2);
  }
};

template<>
struct rsort_compare_numeric<int> {
  bool operator()(int h1, int h2) const {
    return h1 < h2;
  }
};

template<class T>
struct rsort_compare_string {
  bool operator()(const T &h1, const T &h2) const {
    return f$strval(h1).compare(f$strval(h2)) < 0;
  }
};

template<class T>
void f$rsort(array<T> &a, int flag) {
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
void f$asort(array<T> &a, int flag) {
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
void f$arsort(array<T> &a, int flag) {
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
void f$ksort(array<T> &a, int flag) {
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
void f$krsort(array<T> &a, int flag) {
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

template<class T>
double f$array_sum(const array<T> &a) {
  double result = 0;

  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    result += f$floatval(it.get_value());
  }

  return result;
}


template<class T>
var f$getKeyByPos(const array<T> &a, int pos) {
  typename array<T>::const_iterator it = a.middle(pos);
  if (it == a.end()) {
    return var();
  }
  return it.get_key();
}

template<class T>
T f$getValueByPos(const array<T> &a, int pos) {
  typename array<T>::const_iterator it = a.middle(pos);
  if (it == a.end()) {
    return T();
  }
  return it.get_value();
}

template<class T>
array<T> f$create_vector(int n, const T &default_value) {
  array<T> res(array_size(n, 0, true));
  for (int i = 0; i < n; i++) {
    res.push_back(default_value);
  }
  return res;
}

array<var> f$create_vector(int n) {
  array<var> res(array_size(n, 0, true));
  empty_var = var();
  for (int i = 0; i < n; i++) {
    res.push_back(empty_var);
  }
  return res;
}

template<class T>
var f$array_first_key(const array<T> &a) {
  return a.empty() ? var() : a.begin().get_key();
}

template<class T>
T f$array_first_value(const array<T> &a) {
  return a.empty() ? T() : a.begin().get_value(); // in PHP 'false' on empty, here T()
}

template<class T>
var f$array_last_key(const array<T> &a) {
  return a.empty() ? var() : (--a.end()).get_key();
}

template<class T>
T f$array_last_value(const array<T> &a) {
  return a.empty() ? T() : (--a.end()).get_value(); // in PHP 'false' on empty, here T()
}


template<class T>
T vk_dot_product_sparse(const array<T> &a, const array<T> &b) {
  T result = T();
  for (typename array<T>::const_iterator it = a.begin(); it != a.end(); ++it) {
    if (b.isset(it.get_key())) {
      result += it.get_value() * b.get_value(it.get_key());
    }
  }
  return result;
}

template<class T>
T vk_dot_product_dense(const array<T> &a, const array<T> &b) {
  T result = T();
  int size = min(a.count(), b.count());
  for (int i = 0; i < size; i++) {
    result += a.get_value(i) * b.get_value(i);
  }
  return result;
}

template<>
inline int vk_dot_product_dense<int>(const array<int> &a, const array<int> &b) {
  int result = 0;
  int size = min(a.count(), b.count());
  const int *ap = a.get_const_vector_pointer();
  const int *bp = b.get_const_vector_pointer();
  for (int i = 0; i < size; i++) {
    result += ap[i] * bp[i];
  }
  return result;
}

template<>
inline double vk_dot_product_dense<double>(const array<double> &a, const array<double> &b) {
  int size = min(a.count(), b.count());
  const double *ap = a.get_const_vector_pointer();
  const double *bp = b.get_const_vector_pointer();
  return __dot_product(ap, bp, size);
}


template<class T>
T f$vk_dot_product(const array<T> &a, const array<T> &b) {
  if (a.is_vector() && b.is_vector()) {
    return vk_dot_product_dense<T>(a, b);
  }
  return vk_dot_product_sparse<T>(a, b);
}
