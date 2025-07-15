// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <numeric>

#include "common/type_traits/function_traits.h"
#include "common/vector-product.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime/math_functions.h"

template<class T>
array<T> f$array_filter(const array<T>& a) noexcept;

template<class T, class T1>
array<T> f$array_filter(const array<T>& a, const T1& callback) noexcept;

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T>& a, const T1& callback) noexcept;

template<class T>
typename array<T>::key_type f$array_rand(const array<T>& a);

template<class T>
mixed f$array_rand(const array<T>& a, int64_t num);

template<class T1, class T>
array<T> f$array_combine(const array<T1>& keys, const array<T>& values);

template<class T>
void f$shuffle(array<T>& a);

template<class T, class T1>
void f$usort(array<T>& a, const T1& compare) {
  return a.sort(compare, true);
}

template<class T, class T1>
void f$uasort(array<T>& a, const T1& compare) {
  return a.sort(compare, false);
}

template<class T, class T1>
void f$uksort(array<T>& a, const T1& compare) {
  return a.ksort(compare);
}

/*
 *
 *     IMPLEMENTATION
 *
 */

template<class T>
inline void extract_array_column(array<T>& dest, const array<T>& source, const mixed& column_key, const mixed& index_key) {
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
void extract_array_column_instance(array<class_instance<T>>& dest, const array<class_instance<T>>& source, const mixed& column_key, const mixed&) {
  if (source.has_key(column_key)) {
    dest.push_back(source.get_value(column_key));
  }
}

template<class T, class FunT, class ResT = vk::decay_function_arg_t<FunT, 0>>
Optional<ResT> array_column_helper(const array<T>& a, mixed column_key, mixed index_key, FunT&& element_transformer) {
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

  for (const auto& it : a) {
    element_transformer(result, it.get_value(), column_key, index_key);
  }

  return result;
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<array<class_instance<T>>>& a, const mixed& column_key) {
  return array_column_helper(a, column_key, {}, extract_array_column_instance<T>);
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<Optional<array<class_instance<T>>>>& a, const mixed& column_key) {
  auto element_transformer = [](array<class_instance<T>>& dest, const Optional<array<class_instance<T>>>& source, const mixed& column_key,
                                const mixed& index_key) {
    if (source.has_value()) {
      extract_array_column_instance(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, {}, std::move(element_transformer));
}

template<class T>
Optional<array<T>> f$array_column(const array<array<T>>& a, const mixed& column_key, const mixed& index_key = {}) {
  return array_column_helper(a, column_key, index_key, extract_array_column<T>);
}

template<class T>
Optional<array<T>> f$array_column(const array<Optional<array<T>>>& a, const mixed& column_key, const mixed& index_key = {}) {
  auto element_transformer = [](array<T>& dest, const Optional<array<T>>& source, const mixed& column_key, const mixed& index_key) {
    if (source.has_value()) {
      extract_array_column(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

inline Optional<array<mixed>> f$array_column(const array<mixed>& a, const mixed& column_key, const mixed& index_key = {}) {
  auto element_transformer = [](array<mixed>& dest, const mixed& source, const mixed& column_key, const mixed& index_key) {
    if (source.is_array()) {
      extract_array_column(dest, source.as_array(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

template<class T>
inline auto f$array_column(const Optional<T>& a, const mixed& column_key,
                           const mixed& index_key = {}) -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
  if (!a.has_value()) {
    php_warning("first parameter of array_column must be array");
    return false;
  }

  return f$array_column(a.val(), column_key, index_key);
}

template<class T, class F>
array<T> array_filter_impl(const array<T>& a, const F& pred) noexcept {
  array<T> result(a.size());
  for (const auto& it : a) {
    if (pred(it)) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T>
array<T> f$array_filter(const array<T>& a) noexcept {
  return array_filter_impl(a, [](const auto& it) { return f$boolval(it.get_value()); });
}

template<class T, class T1>
array<T> f$array_filter(const array<T>& a, const T1& callback) noexcept {
  return array_filter_impl(a, [&callback](const auto& it) { return f$boolval(callback(it.get_value())); });
}

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T>& a, const T1& callback) noexcept {
  return array_filter_impl(a, [&callback](const auto& it) { return f$boolval(callback(it.get_key())); });
}

template<class T, class CallbackT, class R = typename std::invoke_result_t<std::decay_t<CallbackT>, T>>
array<R> f$array_map(const CallbackT& callback, const array<T>& a) {
  array<R> result(a.size());
  for (const auto& it : a) {
    result.set_value(it.get_key(), callback(it.get_value()));
  }

  return result;
}

template<class R, class T, class CallbackT, class InitialT>
R f$array_reduce(const array<T>& a, const CallbackT& callback, InitialT initial) {
  R result(std::move(initial));
  for (const auto& it : a) {
    result = callback(result, it.get_value());
  }

  return result;
}

template<class T, class T1, class Proj>
array<T> array_diff_impl(const array<T>& a1, const array<T1>& a2, const Proj& projector) {
  array<T> result(a1.size());

  array<int64_t> values{array_size{a2.count(), false}};

  for (const auto& it : a2) {
    values.set_value(projector(it.get_value()), 1);
  }

  for (const auto& it : a1) {
    if (!values.has_key(projector(it.get_value()))) {
      result.set_value(it);
    }
  }
  return result;
}

template<class T, class T1>
std::tuple<typename array<T>::key_type, T> f$array_find(const array<T>& a, const T1& callback) {
  for (const auto& it : a) {
    if (callback(it.get_value())) {
      return std::make_tuple(it.get_key(), it.get_value());
    }
  }

  return {};
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T>& a) {
  if (int64_t size = a.count()) {
    return a.middle(f$mt_rand(0, size - 1)).get_key();
  }
  return {};
}

template<class T>
mixed f$array_rand(const array<T>& a, int64_t num) {
  if (num == 1) {
    return f$array_rand(a);
  }

  int64_t size = a.count();

  if (unlikely(num <= 0 || num > size)) {
    php_warning("Second argument has to be between 1 and the number of elements in the array");
    return {};
  }

  array<typename array<T>::key_type> result(array_size(num, true));
  for (const auto& it : a) {
    if (f$mt_rand(0, --size) < num) {
      result.push_back(it.get_key());
      --num;
    }
  }

  return result;
}

template<class T1, class T>
array<T> f$array_combine(const array<T1>& keys, const array<T>& values) {
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
    const auto& key = it_keys.get_value();
    if (vk::is_type_in_list<T1, string, int64_t>{} || f$is_int(key)) {
      result.set_value(key, it_values.get_value());
    } else {
      result.set_value(f$strval(key), it_values.get_value());
    }
  }

  return result;
}

template<class T>
void f$shuffle(array<T>& a) {
  int64_t n = a.count();
  if (n <= 1) {
    return;
  }

  array<T> result(array_size(n, true));
  const auto& const_arr = a;
  for (const auto& it : const_arr) {
    result.push_back(it.get_value());
  }

  for (int64_t i = 1; i < n; i++) {
    swap(result[i], result[f$mt_rand(0, i)]);
  }

  a = std::move(result);
}

template<class T>
T vk_dot_product_sparse(const array<T>& a, const array<T>& b) {
  T result = T();
  for (const auto& it : a) {
    const auto* b_val = b.find_value(it);
    if (b_val && !f$is_null(*b_val)) {
      result += it.get_value() * (*b_val);
    }
  }
  return result;
}

template<class T>
T vk_dot_product_dense(const array<T>& a, const array<T>& b) {
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  T result = T();
  int64_t size = min(a.count(), b.count());
  for (int64_t i = 0; i < size; i++) {
    result += a.get_value(i) * b.get_value(i);
  }
  return result;
}

template<>
inline int64_t vk_dot_product_dense<int64_t>(const array<int64_t>& a, const array<int64_t>& b) {
  const int64_t size = min(a.count(), b.count());
  const int64_t* ap = a.get_const_vector_pointer();
  const int64_t* bp = b.get_const_vector_pointer();
  return std::inner_product(ap, ap + size, bp, 0L);
}

template<>
inline double vk_dot_product_dense<double>(const array<double>& a, const array<double>& b) {
  int64_t size = min(a.count(), b.count());
  const double* ap = a.get_const_vector_pointer();
  const double* bp = b.get_const_vector_pointer();
  return __dot_product(ap, bp, static_cast<int>(size));
}

template<class T>
T f$vk_dot_product(const array<T>& a, const array<T>& b) {
  if (a.is_vector() && b.is_vector()) {
    return vk_dot_product_dense<T>(a, b);
  }
  return vk_dot_product_sparse<T>(a, b);
}

template<typename Result, typename U, typename Comparator>
Result array_functions_impl_::async_sort([[maybe_unused]] array<U>& arr, [[maybe_unused]] Comparator comparator, [[maybe_unused]] bool renumber) noexcept {
  struct async_sort_stub_class {};
  static_assert(std::is_same_v<Result, async_sort_stub_class>, "array async sort functions supported only in runtime light ");
}

template<typename Result, typename U, typename Comparator>
Result array_functions_impl_::async_ksort([[maybe_unused]] array<U>& arr, [[maybe_unused]] Comparator comparator) noexcept {
  struct async_ksort_stub_class {};
  static_assert(std::is_same_v<Result, async_ksort_stub_class>, "array async sort functions supported only in runtime light ");
}
