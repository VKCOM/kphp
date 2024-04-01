// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_COMMON_ALGO_COMPARE_H
#define ENGINE_COMMON_ALGO_COMPARE_H

#include <algorithm>
#include <iterator>

namespace vk {
template <class Iter1, class Iter2>
int three_way_lexicographical_compare(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2) {
  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    if (*first1 < *first2) {
      return -1;
    } else if (*first2 < *first1) {
      return 1;
    }
  }
  if (first1 != last1) {
    return 1;
  } else if (first2 != last2) {
    return -1;
  } else {
    return 0;
  }
}

template <class Rng1, class Rng2>
int three_way_lexicographical_compare(const Rng1 &range1, const Rng2 &range2) {
  return three_way_lexicographical_compare(std::begin(range1), std::end(range1), //
                                           std::begin(range2), std::end(range2));
}

template<class Range, class Predicate>
bool all_of(const Range &range, Predicate p) {
  return std::all_of(std::begin(range), std::end(range), std::move(p));
}

template<class Range, class Predicate>
bool any_of(const Range &range, Predicate p) {
  return std::any_of(std::begin(range), std::end(range), std::move(p));
}

} // namespace vk

#endif // ENGINE_COMMON_ALGO_COMPARE_H
