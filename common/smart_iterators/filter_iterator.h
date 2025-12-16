// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <iterator>

#include "common/wrappers/function-holder.h"

namespace vk {

template<class Predicate, class Iterator>
class filter_iterator : function_holder<Predicate> {
public:
  using iterator_category =
      typename std::conditional<std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>::value,
                                std::forward_iterator_tag, std::input_iterator_tag>::type;
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  using difference_type = typename std::iterator_traits<Iterator>::difference_type;
  using pointer = typename std::iterator_traits<Iterator>::pointer;
  using reference = typename std::iterator_traits<Iterator>::reference;

  filter_iterator(Predicate f, Iterator x, Iterator end = Iterator())
      : function_holder<Predicate>{std::move(f)},
        iter{std::move(x)},
        end{std::move(end)} {
    if (this->iter != this->end && !(*this)(*this->iter)) {
      ++*this;
    }
  }

  filter_iterator<Predicate, Iterator>& operator++() {
    do {
      ++iter;
    } while (iter != end && !(*this)(*iter));
    return *this;
  }

  bool operator==(const filter_iterator<Predicate, Iterator>& other) const {
    return iter == other.iter;
  }

  bool operator!=(const filter_iterator<Predicate, Iterator>& other) const {
    return iter != other.iter;
  }

  reference operator*() {
    return *iter;
  }

  const reference operator*() const {
    return *iter;
  }

private:
  Iterator iter;
  Iterator end;
};

template<class Predicate, class Iterator>
filter_iterator<Predicate, Iterator> make_filter_iterator(Predicate f, Iterator iter, Iterator end = Iterator()) {
  return {std::move(f), std::move(iter), std::move(end)};
}

} // namespace vk
