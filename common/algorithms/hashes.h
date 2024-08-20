// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef ENGINE_HASHES_H
#define ENGINE_HASHES_H

#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>

#include "common/type_traits/range_value_type.h"
#include "common/wrappers/span.h"

namespace vk {

inline void hash_combine(size_t &seed, size_t new_hash) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const uint32_t r = 47;

  new_hash *= m;
  new_hash ^= new_hash >> r;
  new_hash *= m;

  seed ^= new_hash;
  seed *= m;
  seed += 0xe6546b64;
}

template<class Iter, class Hasher = std::hash<typename std::iterator_traits<Iter>::value_type>>
size_t hash_range(Iter first, Iter last, Hasher hasher = Hasher()) {
  size_t ans = 0;
  for (; first != last; ++first) {
    hash_combine(ans, hasher(*first));
  }
  return ans;
}

template<class Rng, class Hasher = std::hash<range_value_type<Rng>>>
size_t hash_range(const Rng &range, Hasher hasher = Hasher()) {
  return hash_range(std::begin(range), std::end(range), hasher);
}

template<class Rng, class Hasher = std::hash<range_value_type<Rng>>>
class range_hasher : Hasher {
public:
  explicit range_hasher(Hasher hasher = Hasher()) :
    Hasher(std::move(hasher)) {
  }

  size_t operator()(const Rng &range) const {
    return hash_range(range, static_cast<const Hasher &>(*this));
  }
};

template<class T>
size_t std_hash(const T &obj) {
  static_assert(!std::is_same<T, const char *>{}, "You don't want to use std::hash with `const char *`");
  return std::hash<T>{}(obj);
}

template<class ...Ts>
size_t hash_sequence(const Ts &... val) {
  size_t res = 0;
  auto hashes = std::array<size_t, sizeof...(Ts)>{vk::std_hash(val)...};
  for (auto hash : hashes) {
    vk::hash_combine(res, hash);
  }
  return res;
}

} // namespace vk

namespace std {

template<class T1, class T2>
class hash<std::pair<T1, T2>> {
public:
  size_t operator()(const std::pair<T1, T2> & pair) const {
    return vk::hash_sequence(pair.first, pair.second);
  }
};

template<class T>
class hash<vk::span<T>> {
public:
  size_t operator()(const vk::span<T> &span) const {
    return hash_range(span);
  }
};

} // namespace std

#endif // ENGINE_HASHES_H
