// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <tuple>

namespace vk {
template<class Needle>
constexpr bool any_of_equal(Needle const &) {
  return false;
}

template<class Needle, class Arg0, class ...Args>
constexpr bool any_of_equal(Needle const &needle, Arg0 const &arg0, Args const &...args) {
  return needle == arg0 || any_of_equal(needle, args...);
}

template<class Needle>
constexpr bool all_of_equal(Needle const &) {
  return true;
}

template<class Needle, class Arg0, class ...Args>
constexpr bool all_of_equal(Needle const &needle, Arg0 const &arg0, Args const &...args) {
  return needle == arg0 && all_of_equal(needle, args...);
}

template<class Needle, class Arg0, class ...Args>
constexpr bool none_of_equal(Needle const &needle, Arg0 const &arg0, Args const &...args) {
  return !any_of_equal(needle, arg0, args...);
}

template<class T1, class T2, class... Args>
class index_of_type
  : public std::integral_constant<std::size_t, 1 + index_of_type<T1, Args...>::value>
{};

template<class T, class... Args>
class index_of_type<T, T, Args...>
  : public std::integral_constant<std::size_t, 0>
{};

template<std::size_t type_id, class... Args>
using get_nth_type = typename std::tuple_element<type_id, std::tuple<Args...>>::type;

} // namespace vk