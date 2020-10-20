#pragma once

#include <type_traits>

#include "common/type_traits/function_traits.h"

namespace vk {

template<class OnTrueT, class OnFalseT, class ...Args>
typename function_traits<OnTrueT>::ResultType
constexpr_if(std::true_type, OnTrueT &&on_true, OnFalseT &&, Args &&...args) {
  return on_true(std::forward<Args>(args)...);
}

template<class OnTrueT, class OnFalseT, class ...Args>
typename function_traits<OnFalseT>::ResultType
constexpr_if(std::false_type, OnTrueT &&, OnFalseT &&on_false, Args &&...args) {
  return on_false(std::forward<Args>(args)...);
}

} // namespace vk

