// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template <class T, class... Args>
using enable_if_constructible = std::enable_if_t<std::is_constructible<T, Args...>::value>;

} // namespace vk
