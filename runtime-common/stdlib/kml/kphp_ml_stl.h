#pragma once

#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/core/std/containers.h"

namespace kphp_ml::stl {
template<typename T>
using vector = kphp::stl::vector<T, kphp::memory::platform_allocator>;

using string = kphp::stl::string<kphp::memory::platform_allocator>;

template<typename Key, typename T>
using unordered_map = kphp::stl::unordered_map<Key, T, kphp::memory::platform_allocator>;
} // namespace kphp_ml::stl

template<>
struct std::hash<kphp::stl::string<kphp::memory::platform_allocator>> {
  std::size_t operator()(const kphp::stl::string<kphp::memory::platform_allocator>& s) const noexcept {
    std::string_view view{s.c_str(), s.size()};
    return std::hash<std::string_view>{}(view);
  }
};
