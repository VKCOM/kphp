// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>

#include "common/type_traits/range_value_type.h"
#include "common/wrappers/field_getter.h"

namespace vk {

template<
  typename Iter, // ForwardIterator
  typename Proj, // value_type(Iter) -> T
  typename Cmp
  = std::equal_to<std::invoke_result_t<Proj, typename std::iterator_traits<Iter>::reference>>>
Iter adjacent_find(Iter f, Iter l, Proj projector, Cmp cmp = Cmp()) {
  return std::adjacent_find(f, l, [&](const auto &lhs, const auto &rhs) { return cmp(projector(lhs), projector(rhs)); });
}

template<
  typename Iter, // ForwardIterator
  typename Field,
  typename Cmp = std::equal_to<Field>
>
Iter adjacent_find(Iter f, Iter l, Field std::iterator_traits<Iter>::value_type::*field, Cmp cmp = Cmp()) {
  return vk::adjacent_find(f, l, vk::make_field_getter(field), cmp);
}

template<
  typename Rng,
  typename Proj, // value_type(Rng) -> T
  typename Cmp
  = std::equal_to<std::invoke_result_t<Proj, vk::range_value_type<Rng>>>>
auto adjacent_find(Rng &&rng, Proj projector, Cmp cmp = Cmp()) {
  return vk::adjacent_find(std::begin(rng), std::end(rng), projector, cmp);
}

template<
  typename Rng,
  typename Field,
  typename Cmp = std::equal_to<Field>
>
auto adjacent_find(Rng &&rng, Field range_value_type<Rng>::*field, Cmp cmp = Cmp()) {
  return vk::adjacent_find(std::begin(rng), std::end(rng), field, cmp);
}

} // namespace vk
