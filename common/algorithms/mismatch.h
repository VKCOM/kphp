// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>

#include "common/functional/identity.h"
#include "common/type_traits/range_value_type.h"
#include "common/wrappers/field_getter.h"

namespace vk {

template<
  typename Iter1, // ForwardIterator
  typename Iter2, // ForwardIterator
  typename Proj, // value_type(Iter) -> T
  typename Cmp
  = std::equal_to<>>
auto mismatch(Iter1 f, Iter1 l, Iter2 f2, Proj projector, Cmp cmp = Cmp()) {
  return std::mismatch(f, l, f2, [&](const auto &lhs, const auto &rhs) { return cmp(projector(lhs), projector(rhs)); });
}

template<
  typename Iter1, // ForwardIterator
  typename Iter2, // ForwardIterator
  typename Proj, // value_type(Iter) -> T
  typename Cmp
  = std::equal_to<>>
auto mismatch(Iter1 f, Iter1 l, Iter2 f2, Iter2 l2, Proj projector, Cmp cmp = Cmp()) {
  return std::mismatch(f, l, f2, l2, [&](const auto &lhs, const auto &rhs) { return cmp(projector(lhs), projector(rhs)); });
}

template<
  typename Iter1, // ForwardIterator
  typename Iter2, // ForwardIterator
  typename Field,
  typename Cmp = std::equal_to<Field>
>
auto mismatch(Iter1 f, Iter1 l, Iter2 f2, Field std::iterator_traits<Iter1>::value_type::*field, Cmp cmp = Cmp()) {
  return vk::mismatch(f, l, f2, vk::make_field_getter(field), cmp);
}

template<
  typename Iter1, // ForwardIterator
  typename Iter2, // ForwardIterator
  typename Field,
  typename Cmp = std::equal_to<Field>
>
auto mismatch(Iter1 f, Iter1 l, Iter2 f2, Iter2 l2,
              Field std::common_type_t<typename std::iterator_traits<Iter1>::value_type, typename std::iterator_traits<Iter2>::value_type>::*field,
              Cmp cmp = Cmp()) {
  return vk::mismatch(f, l, f2, l2, vk::make_field_getter(field), cmp);
}

template<
  typename Rng1,
  typename Rng2,
  typename Proj = vk::identity, // value_type(Rng) -> T
  typename Cmp = std::equal_to<>
>
auto mismatch(Rng1 &&rng1, Rng2 &&rng2, Proj projector = Proj(), Cmp cmp = Cmp()) {
  return vk::mismatch(std::begin(rng1), std::end(rng1), std::begin(rng2), std::end(rng2), projector, cmp);
}

template<
  typename Rng1,
  typename Rng2,
  typename Field,
  typename Cmp = std::equal_to<Field>
>
auto mismatch(Rng1 &&rng1, Rng2 &&rng2, Field range_value_type<Rng1>::*field, Cmp cmp = Cmp()) {
  return vk::mismatch(std::begin(rng1), std::end(rng1), std::begin(rng2), std::end(rng2), field, cmp);
}

} // namespace vk
