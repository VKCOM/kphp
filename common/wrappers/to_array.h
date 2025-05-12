// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <utility>

namespace vk {
namespace detail {

template<class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&a)[N], std::index_sequence<I...>) {
  return {{a[I]...}};
}

template<class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&&a)[N], std::index_sequence<I...>) {
  return {{std::move(a[I])...}};
}

} // namespace detail

template<class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N]) {
  return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

template<class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&&a)[N]) {
  return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
}

} // namespace vk
