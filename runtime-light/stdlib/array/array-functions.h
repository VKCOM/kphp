// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/math/random-functions.h"

namespace array_functions_impl_ {

template<typename T>
concept convertible_to_php_bool = requires(T t) {
  { f$boolval(t) } -> std::convertible_to<bool>;
};

template<class T, std::invocable<T> F>
requires convertible_to_php_bool<std::invoke_result_t<F, T>>
array<T> array_filter_impl(array<T> a, F f) noexcept {
  array<T> result{a.size()};
  for (const auto& it : std::as_const(a)) {
    if (f$boolval(std::invoke(f, it.get_value()))) {
      result.set_value(it);
    }
  }
  return std::move(result);
}

template<class T, std::invocable<typename array<T>::const_iterator::key_type> F>
requires convertible_to_php_bool<std::invoke_result_t<F, typename array<T>::const_iterator::key_type>>
array<T> array_filter_by_key_impl(array<T> a, F f) noexcept {
  array<T> result{a.size()};
  for (const auto& it : std::as_const(a)) {
    if (f$boolval(std::invoke(f, it.get_key()))) {
      result.set_value(it);
    }
  }
  return std::move(result);
}

template<class T, class F>
requires convertible_to_php_bool<std::invoke_result_t<F, typename array<T>::const_iterator::value_type>>
std::tuple<typename array<T>::const_iterator::key_type, typename array<T>::const_iterator::value_type> array_find_impl(array<T> a, F f) noexcept {
  for (const auto& it : std::as_const(a)) {
    if (std::invoke(f, it.get_value())) {
      return std::tuple{it.get_key(), it.get_value()};
    }
  }
  return std::tuple<typename array<T>::const_iterator::key_type, typename array<T>::const_iterator::value_type>{};
}

} // namespace array_functions_impl_

template<class T>
void f$shuffle(array<T>& arr) noexcept {
  const auto arr_size{arr.count()};
  if (arr_size <= 1) [[unlikely]] {
    return;
  }

  array<T> shuffled{array_size(arr_size, true)};
  for (const auto& it : arr) {
    shuffled.push_back(it.get_value());
  }
  for (int64_t i = 1; i < arr_size; i++) {
    swap(shuffled[i], shuffled[f$mt_rand(0, i)]);
  }

  arr = std::move(shuffled);
}

template<class T>
array<T> f$array_filter(array<T> a) noexcept {
  return array_functions_impl_::array_filter_impl(std::move(a), std::identity{});
}

template<class T, std::invocable<T> F>
array<T> f$array_filter(array<T> a, F f) noexcept {
  return array_functions_impl_::array_filter_impl(std::move(a), std::move(f));
}

template<class T, std::invocable<typename array<T>::const_iterator::key_type> F>
array<T> f$array_filter_by_key(array<T> a, F f) noexcept {
  return array_functions_impl_::array_filter_by_key_impl(std::move(a), std::move(f));
}

template<class T, std::invocable<T> F>
std::tuple<typename array<T>::const_iterator::key_type, typename array<T>::const_iterator::value_type> f$array_find(array<T> a, F f) {
  return array_functions_impl_::array_find_impl(std::move(a), std::move(f));
}

template<class T>
typename array<T>::key_type f$array_rand(const array<T>& a) noexcept {
  if (int64_t size{a.count()}) {
    return a.middle(f$mt_rand(0, size - 1)).get_key();
  }
  return {};
}

template<class T>
mixed f$array_rand(const array<T>& a, int64_t num) noexcept {
  if (num == 1) {
    return f$array_rand(a);
  }

  int64_t size{a.count()};

  if (num <= 0 || num > size) [[unlikely]] {
    kphp::log::warning("second argument has to be between 1 and the number of elements in the array");
    return {};
  }

  array<typename array<T>::key_type> result{array_size(num, true)};
  for (const auto& it : a) {
    if (f$mt_rand(0, --size) < num) {
      result.push_back(it.get_key());
      --num;
    }
  }

  return result;
}

template<class A, std::invocable<A> F, class R = std::invoke_result_t<F, A>>
array<R> f$array_map(F f, array<A> a) noexcept {
  array<R> result{a.size()};
  for (const auto& it : std::as_const(a)) {
    result.set_value(it.get_key(), std::invoke(f, it.get_value()));
  }
  return std::move(result);
}

template<class R, class T, std::invocable<R, T> F, class I>
requires std::constructible_from<R, std::add_rvalue_reference_t<I>>
R f$array_reduce(array<T> a, F f, I init) noexcept {
  R result{R(std::move(init))}; // explicit constructor call to avoid implicit cast
  for (const auto& it : std::as_const(a)) {
    result = std::invoke(f, result, it.get_value());
  }
  return std::move(result);
}

template<class T, class Comparator>
requires(std::invocable<Comparator, T, T>)
void f$usort(array<T>& a, Comparator compare) noexcept {
  a.sort(std::move(compare), true);
}

template<class T, class Comparator>
requires(std::invocable<Comparator, T, T>)
void f$uasort(array<T>& a, Comparator compare) noexcept {
  a.sort(std::move(compare), false);
}

template<class T, class Comparator>
requires(std::invocable<Comparator, typename array<T>::key_type, typename array<T>::key_type>)
void f$uksort(array<T>& a, Comparator compare) noexcept {
  a.ksort(std::move(compare));
}
