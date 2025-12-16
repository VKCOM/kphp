// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <iterator>

#include "common/wrappers/function-holder.h"

namespace vk {

template<class Mapper, class Iterator>
class transform_iterator : function_holder<Mapper> {
private:
  Iterator iter;

public:
  using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;
  using difference_type = typename std::iterator_traits<Iterator>::difference_type;
  using reference = typename std::invoke_result_t<Mapper, typename std::iterator_traits<Iterator>::reference>;
  using value_type = typename std::remove_cv<typename std::remove_reference<reference>::type>::type;
  using pointer = value_type*;

  transform_iterator(Mapper f, Iterator iter)
      : function_holder<Mapper>(std::move(f)),
        iter(std::move(iter)) {}

  transform_iterator<Mapper, Iterator>& operator++() {
    ++iter;
    return *this;
  }

  bool operator==(const transform_iterator<Mapper, Iterator>& other) const {
    return iter == other.iter;
  }

  bool operator!=(const transform_iterator<Mapper, Iterator>& other) const {
    return iter != other.iter;
  }

  reference operator*() const {
    return (*this)(*iter);
  }

  auto operator-(const transform_iterator& rhs) const {
    static_assert(std::is_same<iterator_category, std::random_access_iterator_tag>{}, "random access iterator is expected for operator-");
    return iter - rhs.iter;
  }

  auto operator+(difference_type x) const {
    static_assert(std::is_same<iterator_category, std::random_access_iterator_tag>{}, "random access iterator is expected for operator+");
    return transform_iterator<Mapper, Iterator>(*this, iter + x);
  }

  template<typename dummy = reference>
  typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value, dummy>::type operator[](int x) const {
    return *(*this + x);
  }
};

template<class Mapper, class Iterator>
transform_iterator<std::decay_t<Mapper>, Iterator> make_transform_iterator(Mapper f, Iterator iter) {
  return {std::move(f), std::move(iter)};
}

struct pair_second_getter {
  template<typename FirstType, typename SecondType>
  const SecondType& operator()(const std::pair<FirstType, SecondType>& p) const {
    return p.second;
  }

  template<typename FirstType, typename SecondType>
  SecondType& operator()(std::pair<FirstType, SecondType>& p) const {
    return p.second;
  }
};

} // namespace vk
