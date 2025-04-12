// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template<class... Args>
struct list_of_types;

template<class Arg, class... Args>
struct list_of_types<Arg, Args...> {
  using CurT = Arg;
  using Tail = list_of_types<Args...>;

  template<class T>
  constexpr static bool in_list() {
    return std::is_same<CurT, T>::value || Tail::template in_list<T>();
  }

  template<class T>
  constexpr static bool base_in_list() {
    return std::is_base_of<CurT, T>::value || Tail::template base_in_list<T>();
  }
};

template<>
struct list_of_types<> {
  template<class T>
  constexpr static bool in_list() {
    return false;
  }

  template<class T>
  constexpr static bool base_in_list() {
    return false;
  }
};

template<class T, class... TypeList>
struct is_type_in_list : std::integral_constant<bool, list_of_types<TypeList...>::template in_list<T>()> {};

template<class T, class... TypeList>
struct is_type_in_list<T, vk::list_of_types<TypeList...>> : is_type_in_list<T, TypeList...> {};

template<class T, class ListT, class ResT = void>
using enable_if_in_list = std::enable_if_t<ListT::template in_list<T>(), ResT>;

template<class T, class ListT>
using enable_if_base_in_list = std::enable_if_t<ListT::template base_in_list<T>()>;

} // namespace vk
