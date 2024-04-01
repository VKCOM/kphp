// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <algorithm>

namespace vk {

namespace detail {
template<class C, class T>
auto contains(const C &c, const T &x, int) ->
  std::enable_if_t<std::is_same<decltype(c.find(x)), decltype(std::end(c))>::value, bool> {
  return c.find(x) != std::end(c);
}

template<class C, class T>
auto contains(const C &c, const T &x, long) -> decltype(C::npos, true) {
  return c.find(x) != C::npos;
}

template<class C, class T>
bool contains(const C &v, const T &x, ...) {
  return std::find(std::begin(v), std::end(v), x) != std::end(v);
}

} // namespace detail


template<class C, class T, typename = decltype(std::end(std::declval<const C&>()))>
bool contains(const C& c, const T& x) {
  return detail::contains(c, x, 0);
}

} // namespace vk
