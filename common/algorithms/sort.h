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

template<typename Iter, // RandomAccessIterator
         typename Proj, // value_type(Iter) -> T
         typename Cmp   // StrictWeakOrdering(T, T)
         = std::less<std::invoke_result_t<Proj, typename std::iterator_traits<Iter>::reference>>>
void sort(Iter f, Iter l, Proj projector, Cmp cmp = Cmp()) {
  return std::sort(f, l, [&](const auto &lhs, const auto &rhs) { return cmp(projector(lhs), projector(rhs)); });
}

template<typename Iter, // RandomAccessIterator
         typename Field,
         typename Cmp = std::less<Field> // StrictWeakOrdering(Field, Field)
         >
void sort(Iter f, Iter l, Field std::iterator_traits<Iter>::value_type::*field, Cmp cmp = Cmp()) {
  return vk::sort(f, l, vk::make_field_getter(field), cmp);
}

template<typename Rng,
         typename Proj, // value_type(Rng) -> T
         typename Cmp   // StrictWeakOrdering(T, T)
         = std::less<std::invoke_result_t<Proj, vk::range_value_type<Rng>>>>
void sort(Rng &&rng, Proj projector, Cmp cmp = Cmp()) {
  return vk::sort(std::begin(rng), std::end(rng), projector, cmp);
}

template<typename Rng, typename Field,
         typename Cmp = std::less<Field> // StrictWeakOrdering(Field, Field)
         >
void sort(Rng &&rng, Field range_value_type<Rng>::*field, Cmp cmp = Cmp()) {
  return vk::sort(std::begin(rng), std::end(rng), field, cmp);
}

template<typename Rng>
void sort(Rng &&rng) {
  return std::sort(std::begin(rng), std::end(rng));
}

} // namespace vk
