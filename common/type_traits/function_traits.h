// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "common/algorithms/find.h"
#include "common/type_traits/remove_cv_ref_from_method.h"

namespace vk {

namespace function_traits_detail {

template<typename T>
using clean_function = typename vk::remove_cv_ref_for_method<std::decay_t<T>>::type;

template<class T>
struct function_traits_helper : public function_traits_helper<clean_function<decltype(&T::operator())>> {};

template<class R, class C, class... A>
struct function_traits_helper<R (C::*)(A...)> {
  using ResultType = R;
  using Class = C;
  template<int arg_id>
  using Argument = vk::get_nth_type<arg_id, A...>;
  static constexpr int ArgumentsCount = sizeof...(A);
};

template<class R, class... A>
struct function_traits_helper<R (*)(A...)> {
  using ResultType = R;
  template<int arg_id>
  using Argument = vk::get_nth_type<arg_id, A...>;
  static constexpr int ArgumentsCount = sizeof...(A);
};

} // namespace function_traits_detail

template<typename T>
using function_traits = function_traits_detail::function_traits_helper<function_traits_detail::clean_function<T>>;
template<typename T>
using result_of_t = typename function_traits<T>::ResultType;

template<typename T, int arg_id>
using function_arg_t = typename function_traits<T>::template Argument<arg_id>;

template<typename T, int arg_id>
using decay_function_arg_t = std::decay_t<typename function_traits<T>::template Argument<arg_id>>;

} // namespace vk
