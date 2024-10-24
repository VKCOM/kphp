// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <functional>

// from boost
// see https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
template<typename T>
void hash_combine(size_t &seed, const T &v) noexcept {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
