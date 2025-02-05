// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/array/array-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/math/random-functions.h"
#include "runtime-light/utils/concepts.h"

namespace dl {

template<typename T, typename Comparator>
requires(std::invocable<Comparator, T, T>) task_t<void> async_sort(T *begin_init, T *end_init, Comparator compare) noexcept {
  auto compare_call = [compare]<typename U>(U lhs, U rhs) -> task_t<int64_t> {
    if constexpr (is_async_function_v<Comparator, U, U>) {
      co_return co_await std::invoke(std::move(compare), std::move(lhs), std::move(rhs));
    } else {
      co_return std::invoke(std::move(compare), std::move(lhs), std::move(rhs));
    }
  };
  T *begin_stack[32];
  T *end_stack[32];

  begin_stack[0] = begin_init;
  end_stack[0] = end_init - 1;

  for (int depth = 0; depth >= 0; --depth) {
    T *begin = begin_stack[depth];
    T *end = end_stack[depth];

    while (begin < end) {
      const auto offset = (end - begin) >> 1;
      swap(*begin, begin[offset]);

      T *i = begin + 1;
      T *j = end;

      while (true) {
        while (i < j && (co_await compare_call(*begin, *i)) > 0) {
          i++;
        }

        while (i <= j && (co_await compare_call(*j, *begin)) > 0) {
          j--;
        }

        if (i >= j) {
          break;
        }

        swap(*i++, *j--);
      }

      swap(*begin, *j);

      if (j - begin <= end - j) {
        if (j + 1 < end) {
          begin_stack[depth] = j + 1;
          end_stack[depth++] = end;
        }
        end = j - 1;
      } else {
        if (begin < j - 1) {
          begin_stack[depth] = begin;
          end_stack[depth++] = j - 1;
        }
        begin = j + 1;
      }
    }
  }
  co_return;
}

} // namespace dl

namespace array_functions_impl_ {

template<typename Result, typename U, typename Comparator>
Result async_sort(array<U> &arr, Comparator comparator, bool renumber) noexcept {
  using array_inner = typename array<U>::array_inner;
  using array_bucket = typename array<U>::array_bucket;
  int64_t n = arr.count();

  if (renumber) {
    if (n == 0) {
      co_return;
    }

    if (!arr.is_vector()) {
      array_inner *res = array_inner::create(n, true);
      for (array_bucket *it = arr.p->begin(); it != arr.p->end(); it = arr.p->next(it)) {
        res->push_back_vector_value(it->value);
      }

      arr.p->dispose();
      arr.p = res;
    } else {
      arr.mutate_if_vector_shared();
    }

    const auto elements_cmp = [comparator](U lhs, U rhs) -> task_t<bool> {
      if constexpr (is_async_function_v<Comparator, U, U>) {
        co_return(co_await std::invoke(std::move(comparator), std::move(lhs), std::move(rhs))) > 0;
      } else {
        co_return std::invoke(std::move(comparator), std::move(lhs), std::move(rhs)) > 0;
      }
    };

    U *begin = reinterpret_cast<U *>(arr.p->entries());
    co_await dl::sort<U, decltype(elements_cmp)>(begin, begin + n, elements_cmp);
    co_return;
  }

  if (n <= 1) {
    co_return;
  }

  if (arr.is_vector()) {
    arr.convert_to_map();
  } else {
    arr.mutate_if_map_shared();
  }

  auto **arTmp = static_cast<array_bucket **>(RuntimeAllocator::get().alloc_script_memory(n * sizeof(array_bucket *)));
  uint32_t i = 0;
  for (array_bucket *it = arr.p->begin(); it != arr.p->end(); it = arr.p->next(it)) {
    arTmp[i++] = it;
  }
  php_assert(i == n);

  const auto hash_entry_cmp = [comparator](const array_bucket *lhs, const array_bucket *rhs) -> task_t<bool> {
    if constexpr (is_async_function_v<Comparator, U, U>) {
      co_return(co_await std::invoke(std::move(comparator), lhs->value, rhs->value)) > 0;
    } else {
      co_return std::invoke(std::move(comparator), lhs->value, rhs->value) > 0;
    }
  };
  co_await dl::sort<array_bucket *, decltype(hash_entry_cmp)>(arTmp, arTmp + n, hash_entry_cmp);

  arTmp[0]->prev = arr.p->get_pointer(arr.p->end());
  arr.p->end()->next = arr.p->get_pointer(arTmp[0]);
  for (uint32_t j = 1; j < n; j++) {
    arTmp[j]->prev = arr.p->get_pointer(arTmp[j - 1]);
    arTmp[j - 1]->next = arr.p->get_pointer(arTmp[j]);
  }
  arTmp[n - 1]->next = arr.p->get_pointer(arr.p->end());
  arr.p->end()->prev = arr.p->get_pointer(arTmp[n - 1]);

  RuntimeAllocator::get().free_script_memory(arTmp, n * sizeof(array_bucket *));
}

template<typename Result, typename U, typename Comparator>
Result async_ksort(array<U> &arr, Comparator comparator) noexcept {
  using array_bucket = typename array<U>::array_bucket;
  using key_type = typename array<U>::key_type;
  using list_hash_entry = typename array<U>::list_hash_entry;

  int64_t n = arr.count();
  if (n <= 1) {
    co_return;
  }

  if (arr.is_vector()) {
    arr.convert_to_map();
  } else {
    arr.mutate_if_map_shared();
  }

  array<key_type> keys(array_size(n, true));
  for (auto *it = arr.p->begin(); it != arr.p->end(); it = arr.p->next(it)) {
    keys.p->push_back_vector_value(it->get_key());
  }

  auto *keysp = reinterpret_cast<key_type *>(keys.p->entries());
  co_await dl::sort<key_type, Comparator>(keysp, keysp + n, std::move(comparator));

  auto *prev = static_cast<list_hash_entry *>(arr.p->end());
  for (uint32_t j = 0; j < n; j++) {
    list_hash_entry *cur = nullptr;
    if (arr.is_int_key(keysp[j])) {
      int64_t int_key = keysp[j].to_int();
      uint32_t bucket = arr.p->choose_bucket(int_key);
      while (arr.p->entries()[bucket].int_key != int_key || !arr.p->entries()[bucket].string_key.is_dummy_string()) {
        if (++bucket == arr.p->buf_size) [[unlikely]] {
          bucket = 0;
        }
      }
      cur = static_cast<list_hash_entry *>(&arr.p->entries()[bucket]);
    } else {
      string string_key = keysp[j].to_string();
      int64_t int_key = string_key.hash();
      array_bucket *string_entries = arr.p->entries();
      uint32_t bucket = arr.p->choose_bucket(int_key);
      while (
        (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key.is_dummy_string() || string_entries[bucket].string_key != string_key)) {
        if (++bucket == arr.p->buf_size) [[unlikely]] {
          bucket = 0;
        }
      }
      cur = static_cast<list_hash_entry *>(&string_entries[bucket]);
    }

    cur->prev = arr.p->get_pointer(prev);
    prev->next = arr.p->get_pointer(cur);

    prev = cur;
  }
  prev->next = arr.p->get_pointer(arr.p->end());
  arr.p->end()->prev = arr.p->get_pointer(prev);
}

template<typename T>
concept convertible_to_php_bool = requires(T t) {
  { f$boolval(t) } -> std::convertible_to<bool>;
};

template<class T, class Pred>
requires(std::invocable<Pred, typename array<T>::const_iterator>
           &&convertible_to_php_bool<async_function_unwrapped_return_type_t<Pred, typename array<T>::const_iterator>>)
  task_t<array<T>> array_filter_impl(const array<T> &a, Pred &&pred) noexcept {
  array<T> result{a.size()};
  for (const auto &it : a) {
    bool condition{false};
    if constexpr (is_async_function_v<Pred, typename array<T>::const_iterator>) {
      condition = f$boolval(co_await std::invoke(std::forward<Pred>(pred), it));
    } else {
      condition = f$boolval(std::invoke(std::forward<Pred>(pred), it));
    }
    if (condition) {
      result.set_value(it);
    }
  }
  co_return result;
}

} // namespace array_functions_impl_

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
task_t<array<T>> f$array_filter(array<T> a) noexcept {
  co_return co_await array_functions_impl_::array_filter_impl(a, [](const auto &it) noexcept { return it.get_value(); });
}

template<class T, class Pred>
requires(std::invocable<Pred, T>) task_t<array<T>> f$array_filter(array<T> a, Pred pred) noexcept {
  if constexpr (is_async_function_v<Pred, T>) {
    co_return co_await array_functions_impl_::array_filter_impl(a, [&pred](const auto &it) noexcept -> task_t<bool> {
      co_return co_await std::invoke(std::move(pred), it.get_value());
    });
  } else {
    co_return co_await array_functions_impl_::array_filter_impl(a, [&pred](const auto &it) noexcept {
      return std::invoke(std::move(pred), it.get_value());
    });
  }
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a) noexcept {
  if (int64_t size{a.count()}) {
    return a.middle(f$mt_rand(0, size - 1)).get_key();
  }
  return {};
}

template<class T>
mixed f$array_rand(const array<T> &a, int64_t num) noexcept {
  if (num == 1) {
    return f$array_rand(a);
  }

  int64_t size{a.count()};

  if (num <= 0 || num > size) [[unlikely]] {
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
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2) {
  php_critical_error("call to unsupported function");
}

template<class T, class T1, class T2>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3) {
  php_critical_error("call to unsupported function");
}

template<class T>
array<int64_t> f$array_count_values(const array<T> &a) {
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

template<class T, class Comparator>
requires(std::invocable<Comparator, T, T>) task_t<void> f$usort(array<T> &a, Comparator compare) {
  if constexpr (is_async_function_v<Comparator, T, T>) {
    co_return co_await array_functions_impl_::async_sort<task_t<void>>(a, std::move(compare), true);
  } else {
    co_return a.sort(std::move(compare), true);
  }
}

template<class T, class Comparator>
requires(std::invocable<Comparator, T, T>) task_t<void> f$uasort(array<T> &a, Comparator compare) {
  if constexpr (is_async_function_v<Comparator, T, T>) {
    co_return co_await array_functions_impl_::async_sort<task_t<void>>(a, std::move(compare), false);
  } else {
    co_return a.sort(std::move(compare), false);
  }
}

template<class T, class Comparator>
requires(std::invocable<Comparator, typename array<T>::key_type, typename array<T>::key_type>) task_t<void> f$uksort(array<T> &a, Comparator compare) {
  if constexpr (is_async_function_v<Comparator, T, T>) {
    co_return co_await array_functions_impl_::async_ksort<task_t<void>>(a, std::move(compare));
  } else {
    co_return a.ksort(std::move(compare));
  }
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
void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept {
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
auto f$array_column(const Optional<T> &a, const mixed &column_key, const mixed &index_key = {})
  -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
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
