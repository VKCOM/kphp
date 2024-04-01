// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace vk {

template<typename T, typename Allocator, typename Comp = std::less<T>, typename Equal = std::equal_to<T>>
void sort_and_unique(std::vector<T, Allocator> &v, const Comp &comp = Comp(), const Equal &equal = Equal()) {
  std::sort(v.begin(), v.end(), comp);
  v.erase(std::unique(v.begin(), v.end(), equal), v.end());
}

template<typename T, typename V, typename Allocator, typename Comp = std::less<T>>
bool insert_into_sorted_vector(std::vector<T, Allocator> &v, V &&val, const Comp &comp = Comp()) {
  auto it = std::lower_bound(v.begin(), v.end(), val, comp);
  if (it != v.end() && !comp(val, *it)) {
    return false;
  }
  v.insert(it, std::forward<V>(val));
  return true;
}

template<typename Vector, typename T, typename Comp = std::less<T>>
auto find_in_sorted_vector(Vector &&v, const T &val, const Comp &comp = Comp()) -> decltype(v.end()) {
  auto it = std::lower_bound(v.begin(), v.end(), val, comp);
  return it == v.end() || comp(val, *it) ? v.end() : it;
}

} //   namespace vk
