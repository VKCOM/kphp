// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <functional>
#include <utility>

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

inline constexpr int64_t SORT_REGULAR = 0;
inline constexpr int64_t SORT_NUMERIC = 1;
inline constexpr int64_t SORT_STRING = 2;

template<class T>
string f$implode(const string &s, const array<T> &a) {
  php_critical_error("call to unsupported function");
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

template<class A, std::invocable<A> F, class R = async_function_unwrapped_return_type_t<F, A>>
task_t<array<R>> f$array_map(F &&f, array<A> arr) {
  array<R> result{arr.size()};
  for (const auto &it : arr) {
    if constexpr (is_async_function_v<F, A>) {
      result.set_value(it.get_key(), co_await std::invoke(std::forward<F>(f), it.get_value()));
    } else {
      result.set_value(it.get_key(), std::invoke(std::forward<F>(f), it.get_value()));
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

template<class T>
T f$array_merge(const T &a1) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_merge(const T &a1, const T &a2) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_merge(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(), const T &a7 = T(), const T &a8 = T(),
                const T &a9 = T(), const T &a10 = T(), const T &a11 = T(), const T &a12 = T()) {
  php_critical_error("call to unsupported function");
}

template<class ReturnT, class... Args>
ReturnT f$array_merge_recursive(const Args &...args) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
void f$array_merge_into(T &a, const T1 &another_array) {
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
array<T> f$array_intersect_key(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
array<T> f$array_intersect(const array<T> &a1, const array<T1> &a2) {
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
array<T> f$array_reverse(const array<T> &a, bool preserve_keys = false) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_shift(array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
int64_t f$array_unshift(array<T> &a, const T1 &val) {
  php_critical_error("call to unsupported function");
}

template<class T>
bool f$array_key_exists(int64_t int_key, const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
bool f$array_key_exists(const string &string_key, const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
bool f$array_key_exists(const mixed &v, const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2>
bool f$array_key_exists(const Optional<T1> &v, const array<T2> &a) {
  php_critical_error("call to unsupported function");
}

template<class K, class T, class = vk::enable_if_in_list<K, vk::list_of_types<double, bool>>>
bool f$array_key_exists(K, const array<T> &) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1>
typename array<T>::key_type f$array_search(const T1 &val, const array<T> &a, bool strict = false) {
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
array<typename array<T>::key_type> f$array_keys(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<string> f$array_keys_as_strings(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<int64_t> f$array_keys_as_ints(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_values(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<T> f$array_unique(const array<T> &a, int64_t flags = SORT_STRING) {
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

template<class T, class T1>
bool f$in_array(const T1 &value, const array<T> &a, bool strict = false) {
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

template<class T1, class T2>
int64_t f$array_push(array<T1> &a, const T2 &val) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3, class T4>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3, class T4, class T5>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
int64_t f$array_push(array<T1> &a, const T2 &val2, const T3 &val3, const T4 &val4, const T5 &val5, const T6 &val6) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_pop(array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$array_reserve(array<T> &a, int64_t int_size, int64_t string_size, bool make_vector_if_possible = true) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$array_reserve_vector(array<T> &a, int64_t size) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$array_reserve_map_int_keys(array<T> &a, int64_t size) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$array_reserve_map_string_keys(array<T> &a, int64_t size) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2>
void f$array_reserve_from(array<T1> &a, const array<T2> &base) {
  php_critical_error("call to unsupported function");
}

template<class T>
bool f$array_is_vector(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
bool f$array_is_list(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
void f$shuffle(array<T> &a) {
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
T f$array_unset(array<T> &arr, int64_t key) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_unset(array<T> &arr, const string &key) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$array_unset(array<T> &arr, const mixed &key) {
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
