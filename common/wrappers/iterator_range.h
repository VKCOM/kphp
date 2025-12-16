// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <iterator>
#include <utility>

#include "common/smart_iterators/filter_iterator.h"
#include "common/smart_iterators/take_while_iterator.h"
#include "common/smart_iterators/transform_iterator.h"

namespace vk {

template<class Iter>
class iterator_range {
public:
  using value_type = typename std::iterator_traits<Iter>::value_type;
  using reference_type = typename std::iterator_traits<Iter>::reference;
  using iterator_category = typename std::iterator_traits<Iter>::iterator_category;
  using difference_type = typename std::iterator_traits<Iter>::difference_type;
  using const_reference = typename std::add_const<reference_type>::type;

  using iterator = Iter;
  using const_iterator = iterator;

  iterator_range(iterator begin, iterator end)
      : m_begin{std::move(begin)},
        m_end{std::move(end)} {}

  iterator begin() const {
    return m_begin;
  }

  iterator end() const {
    return m_end;
  }

  const_reference front() const {
    return *begin();
  }

  const_reference back() const {
    auto cur_end = m_end;
    return *(--cur_end);
  }

  template<typename dummy = reference_type>
  typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value, dummy>::type operator[](int x) const {
    return *(m_begin + x);
  }

  iterator_range<std::reverse_iterator<iterator>> get_reversed_range() const {
    return {std::make_reverse_iterator(m_end), std::make_reverse_iterator(m_begin)};
  }

  difference_type size() const {
    return std::distance(m_begin, m_end);
  }

  bool empty() const {
    return m_begin == m_end;
  }

private:
  iterator m_begin;
  iterator m_end;
};

template<class Iter>
iterator_range<Iter> make_iterator_range(Iter begin, Iter end) {
  return iterator_range<Iter>(std::move(begin), std::move(end));
}

template<class Mapper, class Iter>
iterator_range<transform_iterator<std::decay_t<Mapper>, Iter>> make_transform_iterator_range(const Mapper& mapper, Iter begin, Iter end) {
  return {{mapper, std::move(begin)}, {mapper, std::move(end)}};
}

template<class Predicate, class Iter>
iterator_range<filter_iterator<std::decay_t<Predicate>, Iter>> make_filter_iterator_range(const Predicate& pred, Iter begin, Iter end) {
  return {{pred, begin, end}, {pred, end, end}};
}

template<class Predicate, class Iter>
iterator_range<take_while_iterator<std::decay_t<Predicate>, Iter>> make_take_while_iterator_range(const Predicate& pred, Iter begin, Iter end) {
  return {{pred, begin, end}, {pred, end, end}};
}

template<class Predicate, class Cont>
auto filter(const Predicate& pred, const Cont& container) {
  auto rng = make_filter_iterator_range(pred, std::begin(container), std::end(container));
  return Cont{rng.begin(), rng.end()};
}
} // namespace vk
