#pragma once
#include <tuple>
#include <type_traits>

#include "runtime/declarations.h"

template<typename T, typename T1>
using enable_if_constructible_or_unknown = typename std::enable_if<std::is_same<T1, Unknown>::value || std::is_constructible<T, T1>::value>::type;

using list_bool_int_double = vk::list_of_types<bool, int, double>;

using list_bool_int_double_array = vk::list_of_types<bool, int, double, array_tag>;

template<class T>
using enable_for_bool_int_double = vk::enable_if_in_list<T, list_bool_int_double>;

template<class T>
using enable_for_bool_int_double_array = vk::enable_if_base_in_list<T, list_bool_int_double_array>;

template<typename T>
struct is_class_instance_inside : std::false_type {
};

template<typename I>
struct is_class_instance_inside<class_instance<I>> : std::true_type {
};

template<typename T>
struct is_class_instance_inside<OrFalse<T>> : is_class_instance_inside<T> {
};

template<typename T>
struct is_class_instance_inside<array<T>> : is_class_instance_inside<T> {
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
