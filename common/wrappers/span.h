// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_SPAN_H
#define ENGINE_SPAN_H

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "common/wrappers/is_contiguous_iterator.h"
#include "common/wrappers/iterator_range.h"

namespace vk {

namespace details {
inline auto to_pointer(std::nullptr_t x) {
  return x;
}
template<typename T>
auto to_pointer(T* x) {
  return x;
}
template<typename T>
auto to_pointer(T x) {
  return to_pointer(x.operator->());
}
} // namespace details

template<class T>
struct span {
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using reverse_iterator = std::reverse_iterator<T*>;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  template<typename TT>
  using enable_if_is_good_iterator = std::enable_if_t<is_contiguous_iterator_of_type<value_type, TT>{}>;

  span() noexcept
      : _data{nullptr},
        _size{0} {}

  template<size_t N>
  span(T (&arr)[N])
      : _data{arr},
        _size{N} {}

  template<typename It, typename = enable_if_is_good_iterator<It>>
  span(It data, size_t size)
      : _data{details::to_pointer(data)},
        _size{size} {}

  template<typename C, typename = enable_if_is_good_iterator<decltype(std::declval<C&>().begin())>>
  span(C& v)
      : span(v.begin(), v.end()) {}

  template<typename C, typename = enable_if_is_good_iterator<decltype(std::declval<const C&>().begin())>>
  span(const C& v)
      : span(v.begin(), v.end()) {}

  template<typename It, typename = enable_if_is_good_iterator<It>>
  span(It first, It last)
      : span(first, std::distance(first, last)) {
    assert(last >= first);
  }

  span(const span<T>&) = default;

  span& operator=(const span<T>&) = default;

  bool operator==(span<T> other) const noexcept {
    return std::equal(begin(), end(), other.begin(), other.end());
  }

  bool operator<(span<T> other) const noexcept {
    return std::lexicographical_compare(this->begin(), this->end(), other.begin(), other.end());
  }

  bool operator<=(span<T> other) const noexcept {
    return !(other < *this);
  }

  T& operator[](size_t i) const noexcept {
    return data()[i];
  }

  explicit operator std::vector<typename std::remove_cv<T>::type>() const noexcept {
    return {begin(), end()};
  }

  T* begin() const noexcept {
    return data();
  }

  T* end() const noexcept {
    return data() + size();
  }

  std::reverse_iterator<T*> rbegin() const noexcept {
    return std::reverse_iterator<T*>(end());
  }

  std::reverse_iterator<T*> rend() const noexcept {
    return std::reverse_iterator<T*>(begin());
  }

  size_t size() const noexcept {
    return _size;
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  T* data() const noexcept {
    return _data;
  }

  operator span<const T>() {
    return {data(), size()};
  }

  T& back() const noexcept {
    return *(end() - 1);
  }

  T& front() const noexcept {
    return *begin();
  }

  span subspan(std::size_t offset) const {
    return {data() + offset, size() - offset};
  }

  span subspan(std::size_t offset, size_t count) const {
    return {data() + offset, count};
  }

  span first(size_t count) const {
    return subspan(0, count);
  }

  span last(size_t count) const {
    return subspan(size() - count, count);
  }

  iterator_range<T*> to_range() const noexcept {
    return {begin(), end()};
  }

private:
  T* _data;
  size_t _size;
};

} // namespace vk

#endif // ENGINE_SPAN_H
