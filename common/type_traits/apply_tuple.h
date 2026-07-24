//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <tuple>

namespace vk {

template<template<typename...> typename Template, typename Tuple>
struct apply_tuple;

template<template<typename...> typename Template, typename... Args>
struct apply_tuple<Template, std::tuple<Args...>> {
  using Type = Template<Args...>;
};

template<template<typename...> typename Template, typename Tuple>
using apply_tuple_t = typename apply_tuple<Template, Tuple>::Type;

} // namespace vk
