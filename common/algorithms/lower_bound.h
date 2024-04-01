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
         typename Proj, // value_type(Iter) -> U
         typename T,
         typename Cmp // StrictWeakOrdering(U, U)
         = std::less<typename std::invoke_result_t<Proj, typename std::iterator_traits<Iter>::reference>>>
Iter lower_bound(Iter f, Iter l, const T &val, Proj projector, Cmp cmp = Cmp()) {
  return std::lower_bound(f, l, val, [&](const auto &lhs, const T &rhs) { return cmp(projector(lhs), rhs); });
}

template<typename Iter, // RandomAccessIterator
         typename Field, typename T,
         typename Cmp = std::less<Field> // StrictWeakOrdering(Field, Field)
         >
Iter lower_bound(Iter f, Iter l, const T &val, Field std::iterator_traits<Iter>::value_type::*field, Cmp cmp = Cmp()) {
  return vk::lower_bound(f, l, val, vk::make_field_getter(field), cmp);
}

template<typename Rng, typename T,
         typename Proj, // value_type(Rng) -> T
         typename Cmp   // StrictWeakOrdering(T, T)
         = std::less<typename std::invoke_result_t<Proj, vk::range_value_type<Rng>>>>
auto lower_bound(Rng &&rng, const T &val, Proj projector, Cmp cmp = Cmp()) -> decltype(std::begin(rng)) {
  return vk::lower_bound(std::begin(rng), std::end(rng), val, projector, cmp);
}

template<typename Rng,
         typename Field, // field type
         typename T,
         typename Cmp = std::less<Field> // StrictWeakOrdering(Field, Field)
         >
auto lower_bound(Rng &&rng, const T &val, Field range_value_type<Rng>::*field, Cmp cmp = Cmp()) -> decltype(std::begin(rng)) {
  return vk::lower_bound(std::begin(rng), std::end(rng), val, field, cmp);
}
} // namespace vk
