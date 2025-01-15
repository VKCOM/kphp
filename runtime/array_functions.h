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

namespace dl {

template<typename T, typename Comparator>
void sort(T *begin_init, T *end_init, Comparator &&compare) {
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

      T *i = begin + 1, *j = end;

      while (1) {
        while (i < j && std::invoke(std::forward<Comparator>(compare), *begin, *i) > 0) {
          i++;
        }

        while (i <= j && std::invoke(std::forward<Comparator>(compare), *j, *begin) > 0) {
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
}

} // namespace dl

namespace array_functions_impl_ {

template<typename Result, typename U, typename Comparator>
Result sort(array<U> &arr, Comparator &&comparator, bool renumber) {
    using array_inner = typename array<U>::array_inner;
    using array_bucket = typename array<U>::array_bucket;
    int64_t n = arr.count();

    if (renumber) {
      if (n == 0) {
        return;
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

      auto elements_cmp = [&comparator](const U &lhs, const U &rhs)
      { return std::invoke(std::forward<Comparator>(comparator), lhs, rhs) > 0; };
      U *begin = reinterpret_cast<U *>(arr.p->entries());
      dl::sort<U, decltype(elements_cmp)>(begin, begin + n, std::move(elements_cmp));
      return;
    }

    if (n <= 1) {
      return;
    }

    if (arr.is_vector()) {
      arr.convert_to_map();
    } else {
      arr.mutate_if_map_shared();
    }

    array_bucket **arTmp = (array_bucket **)RuntimeAllocator::get().alloc_script_memory(n * sizeof(array_bucket *));
    uint32_t i = 0;
    for (array_bucket *it = arr.p->begin(); it != arr.p->end(); it = arr.p->next(it)) {
      arTmp[i++] = it;
    }
    php_assert(i == n);

    auto hash_entry_cmp = [&comparator](const array_bucket *lhs, const array_bucket *rhs)
    { return std::invoke(std::forward<Comparator>(comparator), lhs->value, rhs->value) > 0; };
    dl::sort<array_bucket *, decltype(hash_entry_cmp)>(arTmp, arTmp + n, std::move(hash_entry_cmp));

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
Result ksort(array<U> &arr, Comparator &&comparator) {
  using array_bucket = typename array<U>::array_bucket;
  using key_type = typename array<U>::key_type;
  using list_hash_entry = typename array<U>::list_hash_entry;

  int64_t n = arr.count();
  if (n <= 1) {
    return;
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

  key_type *keysp = (key_type *)keys.p->entries();
  dl::sort<key_type, Comparator>(keysp, keysp + n, std::forward<Comparator>(comparator));

  list_hash_entry *prev = (list_hash_entry *)arr.p->end();
  for (uint32_t j = 0; j < n; j++) {
    list_hash_entry *cur;
    if (arr.is_int_key(keysp[j])) {
      int64_t int_key = keysp[j].to_int();
      uint32_t bucket = arr.p->choose_bucket(int_key);
      while (arr.p->entries()[bucket].int_key != int_key || !arr.p->entries()[bucket].string_key.is_dummy_string()) {
        if (unlikely(++bucket == arr.p->buf_size)) {
          bucket = 0;
        }
      }
      cur = (list_hash_entry *)&arr.p->entries()[bucket];
    } else {
      string string_key = keysp[j].to_string();
      int64_t int_key = string_key.hash();
      array_bucket *string_entries = arr.p->entries();
      uint32_t bucket = arr.p->choose_bucket(int_key);
      while (
        (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key.is_dummy_string() || string_entries[bucket].string_key != string_key)) {
        if (unlikely(++bucket == arr.p->buf_size)) {
          bucket = 0;
        }
      }
      cur = (list_hash_entry *)&string_entries[bucket];
    }

    cur->prev = arr.p->get_pointer(prev);
    prev->next = arr.p->get_pointer(cur);

    prev = cur;
  }
  prev->next = arr.p->get_pointer(arr.p->end());
  arr.p->end()->prev = arr.p->get_pointer(prev);
}
}

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
T f$array_merge_spread(const T &a1, const T &a2, const T &a3, const T &a4 = T(), const T &a5 = T(), const T &a6 = T(), const T &a7 = T(), const T &a8 = T(),
                       const T &a9 = T(), const T &a10 = T(), const T &a11 = T(), const T &a12 = T());

template<class T, class T1>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2);

template<class T, class T1, class T2>
array<T> f$array_intersect_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3);

template<class T, class T1>
array<T> f$array_diff_key(const array<T> &a1, const array<T1> &a2);

template<class T, class T1>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2);

template<class T, class T1, class T2>
array<T> f$array_diff_assoc(const array<T> &a1, const array<T1> &a2, const array<T2> &a3);

template<class T>
typename array<T>::key_type f$array_rand(const array<T> &a);

template<class T>
mixed f$array_rand(const array<T> &a, int64_t num);

template<class T>
array<int64_t> f$array_count_values(const array<T> &a);

template<class T>
array<T> f$array_fill(int64_t start_index, int64_t num, const T &value);

template<class T1, class T>
array<T> f$array_fill_keys(const array<T1> &keys, const T &value);

template<class T1, class T>
array<T> f$array_combine(const array<T1> &keys, const array<T> &values);

template<class T>
void f$shuffle(array<T> &a);

template<class T, class T1>
void f$usort(array<T> &a, const T1 &compare) {
  return array_functions_impl_::sort<void>(a, compare, true);
}

template<class T, class T1>
void f$uasort(array<T> &a, const T1 &compare) {
  return array_functions_impl_::sort<void>(a, compare, false);
}

template<class T, class T1>
void f$uksort(array<T> &a, const T1 &compare) {
  return array_functions_impl_::ksort<void>(a, compare);
}

template<class T>
mixed f$getKeyByPos(const array<T> &a, int64_t pos);

template<class T>
T f$getValueByPos(const array<T> &a, int64_t pos);

template<class T>
inline array<T> f$create_vector(int64_t n, const T &default_value);

template<class T>
mixed f$array_first_key(const array<T> &a);

template<class T>
inline void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept;

/*
 *
 *     IMPLEMENTATION
 *
 */

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
  auto element_transformer = [](array<class_instance<T>> &dest, const Optional<array<class_instance<T>>> &source, const mixed &column_key,
                                const mixed &index_key) {
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
  auto element_transformer = [](array<T> &dest, const Optional<array<T>> &source, const mixed &column_key, const mixed &index_key) {
    if (source.has_value()) {
      extract_array_column(dest, source.val(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

inline Optional<array<mixed>> f$array_column(const array<mixed> &a, const mixed &column_key, const mixed &index_key = {}) {
  auto element_transformer = [](array<mixed> &dest, const mixed &source, const mixed &column_key, const mixed &index_key) {
    if (source.is_array()) {
      extract_array_column(dest, source.as_array(), column_key, index_key);
    }
  };

  return array_column_helper(a, column_key, index_key, std::move(element_transformer));
}

template<class T>
inline auto f$array_column(const Optional<T> &a, const mixed &column_key, const mixed &index_key = {})
  -> decltype(f$array_column(std::declval<T>(), column_key, index_key)) {
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
  return array_filter_impl(a, [](const auto &it) { return f$boolval(it.get_value()); });
}

template<class T, class T1>
array<T> f$array_filter(const array<T> &a, const T1 &callback) noexcept {
  return array_filter_impl(a, [&callback](const auto &it) { return f$boolval(callback(it.get_value())); });
}

template<class T, class T1>
array<T> f$array_filter_by_key(const array<T> &a, const T1 &callback) noexcept {
  return array_filter_impl(a, [&callback](const auto &it) { return f$boolval(callback(it.get_key())); });
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
T f$array_merge_spread(const T &a1, const T &a2, const T &a3, const T &a4, const T &a5, const T &a6, const T &a7, const T &a8, const T &a9, const T &a10,
                       const T &a11, const T &a12) {
  if (!a1.is_vector() || !a2.is_vector() || !a3.is_vector() || !a4.is_vector() || !a5.is_vector() || !a6.is_vector() || !a7.is_vector() || !a8.is_vector()
      || !a9.is_vector() || !a10.is_vector() || !a11.is_vector() || !a12.is_vector()) {
    php_warning("Cannot unpack array with string keys");
  }
  return f$array_merge(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

template<class ReturnT, class... Args>
ReturnT f$array_merge_recursive(const Args &...args) {
  array<mixed> result{(args.size() + ... + array_size{})};
  (result.merge_with_recursive(args), ...);
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

template<class T, class T1>
std::tuple<typename array<T>::key_type, T> f$array_find(const array<T> &a, const T1 &callback) {
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

template<class T>
array<int64_t> f$array_count_values(const array<T> &a) {
  array<int64_t> result(array_size(a.count(), false));

  for (const auto &it : a) {
    ++result[f$strval(it.get_value())];
  }
  return result;
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
mixed f$array_key_last(const array<T> &a) {
  return f$array_last_key(a);
}

template<class T>
void f$array_swap_int_keys(array<T> &a, int64_t idx1, int64_t idx2) noexcept {
  a.swap_int_keys(idx1, idx2);
}

template<class T>
void f$sort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare<T>(), true);
    case SORT_NUMERIC:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare_numeric<T>(), true);
    case SORT_STRING:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare_string<T>(), true);
    default:
      php_warning("Unsupported sort_flag in function sort");
  }
}

template<class T>
void f$rsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare<T>(), true);
    case SORT_NUMERIC:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare_numeric<T>(), true);
    case SORT_STRING:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare_string<T>(), true);
    default:
      php_warning("Unsupported sort_flag in function rsort");
  }
}

template<class T>
void f$arsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare<T>(), false);
    case SORT_NUMERIC:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare_numeric<T>(), false);
    case SORT_STRING:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::rsort_compare_string<T>(), false);
    default:
      php_warning("Unsupported sort_flag in function arsort");
  }
}

template<class T>
void f$ksort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::sort_compare<typename array<T>::key_type>());
    case SORT_NUMERIC:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::sort_compare_numeric<typename array<T>::key_type>());
    case SORT_STRING:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::sort_compare_string<typename array<T>::key_type>());
    default:
      php_warning("Unsupported sort_flag in function ksort");
  }
}

template<class T>
void f$krsort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::rsort_compare<typename array<T>::key_type>());
    case SORT_NUMERIC:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::rsort_compare_numeric<typename array<T>::key_type>());
    case SORT_STRING:
      return array_functions_impl_::ksort<void>(a, array_functions_impl_::rsort_compare_string<typename array<T>::key_type>());
    default:
      php_warning("Unsupported sort_flag in function krsort");
  }
}

template<class T>
void f$asort(array<T> &a, int64_t flag = SORT_REGULAR) {
  switch (flag) {
    case SORT_REGULAR:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare<T>(), false);
    case SORT_NUMERIC:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare_numeric<T>(), false);
    case SORT_STRING:
      return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare_string<T>(), false);
    default:
      php_warning("Unsupported sort_flag in function asort");
  }
}

template<class T>
void f$natsort(array<T> &a) {
  return array_functions_impl_::sort<void>(a, array_functions_impl_::sort_compare_natural<typename array<T>::key_type>(), false);
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
