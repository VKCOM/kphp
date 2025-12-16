// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <iterator>
#include <utility>

#include "common/wrappers/function-holder.h"

namespace vk {

template<class Predicate, class Iterator>
class take_while_iterator : function_holder<Predicate> {
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  using difference_type = typename std::iterator_traits<Iterator>::difference_type;
  using pointer = typename std::iterator_traits<Iterator>::pointer;
  using reference = typename std::iterator_traits<Iterator>::reference;
  take_while_iterator(Predicate f, Iterator x, Iterator end = Iterator())
      : function_holder<Predicate>{std::move(f)},
        iter{std::move(x)},
        end{std::move(end)} {
    if (iter != end && !(*this)(*iter)) {
      iter = end;
    }
  }

  take_while_iterator<Predicate, Iterator>& operator++() {
    ++iter;
    if (iter != end && !(*this)(*iter)) {
      iter = end;
    }
    return *this;
  }

  bool operator==(const take_while_iterator<Predicate, Iterator>& other) const {
    return iter == other.iter;
  }

  bool operator!=(const take_while_iterator<Predicate, Iterator>& other) const {
    return iter != other.iter;
  }

  reference operator*() {
    return *iter;
  }

private:
  Iterator iter;
  Iterator end;
};

template<class Predicate, class Iterator>
take_while_iterator<std::decay_t<Predicate>, Iterator> make_take_while_iterator(Predicate f, Iterator iter, Iterator end = Iterator()) {
  return {std::move(f), std::move(iter), std::move(end)};
}

} // namespace vk
