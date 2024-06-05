// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <tuple>
#include <type_traits>

#include "common/type_traits/list_of_types.h"

#include "runtime-core/kphp-types/decl/declarations.h"
#include "runtime-core/kphp-types/decl/optional.h"
#include "runtime-core/kphp-types/decl/shape.h"

template <typename, template <typename...> class>
struct is_instance_of : std::false_type {};

template <template <typename...> class U, typename... Args>
struct is_instance_of<U<Args...>, U> : std::true_type {};

template <typename, template <typename...> class>
struct is_instance_of_const_ref : std::false_type {};

template <template <typename...> class U, typename... Args>
struct is_instance_of_const_ref<const U<Args...> &, U> : std::true_type {};

template<typename T>
struct is_array : is_instance_of<T, __array> {
};

template<typename T, typename T1>
struct is_constructible_or_unknown : std::is_constructible<T, T1> {
};

template<typename T>
struct is_constructible_or_unknown<T, Unknown> : std::true_type {
};

template<typename T, typename T1>
using enable_if_constructible_or_unknown = std::enable_if_t<is_constructible_or_unknown<T, T1>::value>;

template<class T>
using enable_for_bool_int_double = vk::enable_if_in_list<T, vk::list_of_types<bool, int, int64_t, double>>;

template<typename>
struct is_class_instance : std::false_type {
};

template<typename T, typename Allocator>
struct is_class_instance<__class_instance<T, Allocator>> : std::true_type {
};

template<typename T>
struct is_class_instance_inside : is_class_instance<T> {
};

template<typename T>
struct is_class_instance_inside<Optional<T>> : is_class_instance_inside<T> {
};

template<typename T, typename Allocator>
struct is_class_instance_inside<__array<T, Allocator>> : is_class_instance_inside<T> {
};

template<typename U>
struct is_class_instance_inside<std::tuple<U>> : is_class_instance_inside<U> {
};

template<typename U, typename... Ts>
struct is_class_instance_inside<std::tuple<U, Ts...>> :
  std::integral_constant<bool,
    is_class_instance_inside<U>::value ||
    is_class_instance_inside<std::tuple<Ts...>>::value> {
};

template<size_t ...Is, typename ...Ts>
struct is_class_instance_inside<shape<std::index_sequence<Is...>, Ts...>> :
  is_class_instance_inside<std::tuple<Ts...>> {
};

template<class T, class U>
struct one_of_is_unknown : vk::is_type_in_list<Unknown, T, U> {
};

template<class T, class U>
using enable_if_one_of_types_is_unknown = std::enable_if_t<one_of_is_unknown<T, U>{}, bool>;

template<class T, class U>
using disable_if_one_of_types_is_unknown = std::enable_if_t<!one_of_is_unknown<T, U>{} && !(is_class_instance<T>{} && is_class_instance<U>{}), bool>;

template<class T, class ResT = void>
using enable_if_t_is_optional = std::enable_if_t<is_optional<std::decay_t<T>>::value, ResT>;

template<class T, class ResT = void>
using enable_if_t_is_not_optional = std::enable_if_t<!is_optional<std::decay_t<T>>{}, ResT>;

template<class T, class T2>
using enable_if_t_is_optional_t2 = std::enable_if_t<std::is_same<T, Optional<T2>>::value>;

template<class T, typename Allocator>
using enable_if_t_is_optional_string = enable_if_t_is_optional_t2<T, __runtime_core::string<Allocator>>;

