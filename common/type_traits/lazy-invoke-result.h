// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template<typename F, typename... Args>
struct lazy_invoke_result {
  using type = std::invoke_result_t<F, Args...>;
};

template<typename F, typename... Args>
using lazy_invoke_result_t = typename lazy_invoke_result<F, Args...>::type;

} // namespace vk
