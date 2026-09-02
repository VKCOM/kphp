// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template<bool Condition, typename IfTrueLazy, typename IfFalseLazy>
struct lazy_conditional {
  using type = typename std::conditional<Condition, IfTrueLazy, IfFalseLazy>::type::type;
};

template<bool Condition, typename IfTrueLazy, typename IfFalseLazy>
using lazy_conditional_t = typename vk::lazy_conditional<Condition, IfTrueLazy, IfFalseLazy>::type;

} // namespace vk
