// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template<class T>
struct is_trivially_copyable {
#if __GNUG__ && __GNUC__ < 5
  // works identically in common cases, but there are subtle differneces: https://stackoverflow.com/questions/12754886/has-trivial-copy-behaves-differently-in-clang-and-gcc-whos-right
  static constexpr bool value = __is_trivially_copyable(T);
#else
  static constexpr bool value = std::is_trivially_copyable<T>::value;
#endif
};

} // namespace vk
