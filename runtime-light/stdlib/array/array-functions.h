// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/math/random-functions.h"

template<class T>
void f$shuffle(array<T> &arr) noexcept {
  const auto arr_size{arr.count()};
  if (arr_size <= 1) [[unlikely]] {
    return;
  }

  array<T> shuffled{array_size(arr_size, true)};
  for (const auto &it : arr) {
    shuffled.push_back(it.get_value());
  }
  for (int64_t i = 1; i < arr_size; i++) {
    swap(shuffled[i], shuffled[f$mt_rand(0, i)]);
  }

  arr = std::move(shuffled);
}

template<class T>
array<array<T>> f$array_chunk(const array<T> &a, int64_t chunk_size, bool preserve_keys = false) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_slice(const array<T> &a, int64_t offset, const mixed &length_var = mixed(), bool preserve_keys = false) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length, const array<Unknown> &) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1 = T>
array<T> f$array_splice(array<T> &a, int64_t offset, int64_t length = std::numeric_limits<int64_t>::max(), const array<T1> &replacement = array<T1>()) {
  php_critical_error("call to unsupported function");
}

template<class ReturnT, class InputArrayT, class DefaultValueT>
ReturnT f$array_pad(const array<InputArrayT> &a, int64_t size, const DefaultValueT &default_value) {
  php_critical_error("call to unsupported function");
}

template<class ReturnT, class DefaultValueT>
ReturnT f$array_pad(const array<Unknown> &a, int64_t size, const DefaultValueT &default_value) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_filter(const array<T> &a) noexcept {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback) noexcept {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T> &a, const T1 &callback) noexcept {
  php_critical_error("call to unsupported function");
}

/**
 * Currently, array_map is always considered async. Despite we rely on symmetric transfer optimization,
 * we need to be careful with such functions. We may want to split such functions into sync and async
 * versions in case we face with performance problems.
 */
template<class A, std::invocable<A> F, class R = async_function_unwrapped_return_type_t<F, A>>
task_t<array<R>> f$array_map(F f, array<A> arr) noexcept {
  array<R> result{arr.size()};
  for (const auto &it : arr) {
    if constexpr (is_async_function_v<F, A>) {
      result.set_value(it.get_key(), co_await std::invoke(f, it.get_value()));
    } else {
      result.set_value(it.get_key(), std::invoke(f, it.get_value()));
    }
  }
  co_return result;
}

template<class R, class T, class CallbackT, class InitialT>
R f$array_reduce(const array<T> &a, const CallbackT &callback, InitialT initial) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_merge_spread(const T &a1) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_merge_spread(const T &a1, const T &a2) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_merge_spread(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(), const T &a7 = T(), const T &a8 = T(),
                       const T &a9 = T(), const T &a10 = T(), const T &a11 = T(), const T &a12 = T()) {
  php_critical_error("call to unsupported function");
}

template<class ReturnT, class... Args>
ReturnT f$array_merge_recursive(const Args &...args) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_replace(const T &base_array, const T &replacements = T()) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_replace(const T &base_array, const T &replacements_1, const T &replacements_2, const T &replacements_3 = T(), const T &replacements_4 = T(),
                  const T &replacements_5 = T(), const T &replacements_6 = T(), const T &replacements_7 = T(), const T &replacements_8 = T(),
                  const T &replacements_9 = T(), const T &replacements_10 = T(), const T &replacements_11 = T()) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1, class T2>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_diff_key(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1, class T2>
array<T> f$array_diff(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1, class T2>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  php_critical_error("call to unsupported function");
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$array_rand(const array<T> &a, int64_t num) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<int64_t> f$array_count_values(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<typename array<T>::key_type> f$array_flip(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_fill(int64_t start_index, int64_t num, const T &value) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &keys, const T &value) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$sort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$rsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
void f$usort(array<T> &a, const T1 &compare) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$asort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$arsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
void f$uasort(array<T> &a, const T1 &compare) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$ksort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$krsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
void f$uksort(array<T> &a, const T1 &compare) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$natsort(array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T, class ReturnT = std::conditional_t<std::is_same<T, int64_t>{}, int64_t, double>>
ReturnT f$array_sum(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$getKeyByPos(const array<T> &a, int64_t pos) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$getValueByPos(const array<T> &a, int64_t pos) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$create_vector(int64_t n, const T &default_value) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$array_first_key(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_first_value(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$array_last_key(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_last_value(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept {
  php_critical_error("call to unsupported function");
}

template<class T>
array<mixed> f$to_array_debug(const class_instance<T> &klass, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<class... Args>
array<mixed> f$to_array_debug(const std::tuple<Args...> &tuple, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<size_t... Indexes, typename... T>
array<mixed> f$to_array_debug(const shape<std::index_sequence<Indexes...>, T...> &shape, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<mixed> f$instance_to_array(const class_instance<T> &klass, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<class... Args>
array<mixed> f$instance_to_array(const std::tuple<Args...> &tuple, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<size_t... Indexes, typename... T>
array<mixed> f$instance_to_array(const shape<std::index_sequence<Indexes...>, T...> &shape, bool with_class_names = false) {
  php_critical_error("call to unsupported function");
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<array<class_instance<T>>> &a, const mixed &column_key) {
  php_critical_error("call to unsupported function");
}

template<class T>
Optional<array<class_instance<T>>> f$array_column(const array<Optional<array<class_instance<T>>>> &a, const mixed &column_key) {
  php_critical_error("call to unsupported function");
}

template<class T>
Optional<array<T>> f$array_column(const array<array<T>> &a, const mixed &column_key, const mixed &index_key = {}) {
  php_critical_error("call to unsupported function");
}

template<class T>
Optional<array<T>> f$array_column(const array<Optional<array<T>>> &a, const mixed &column_key, const mixed &index_key = {}) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$array_column(const array<mixed> &a, const mixed &column_key, const mixed &index_key = {}) {
  php_critical_error("call to unsupported function");
}

template<class T>
auto f$array_column(const Optional<T> &a, const mixed &column_key,
                    const mixed &index_key = {}) -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$array_key_first(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$array_key_last(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
std::tuple<typename array<T>::key_type, T> f$array_find(const array<T> &a, const T1 &callback) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$vk_dot_product(const array<T> &a, const array<T> &b) {
  php_critical_error("call to unsupported function");
}
