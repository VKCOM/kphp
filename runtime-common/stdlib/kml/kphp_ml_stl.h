#pragma once

#include "runtime-common/core/allocator/platform-allocator.h"
#include "runtime-common/core/std/containers.h"
#include <memory>

namespace kphp_ml::stl {
template<typename T>
using vector = kphp::stl::vector<T, kphp::memory::platform_allocator>;

using string = kphp::stl::string<kphp::memory::platform_allocator>;

template<typename Key, typename T>
using unordered_map = kphp::stl::unordered_map<Key, T, kphp::memory::platform_allocator>;

template<typename T, typename Deleter>
using unique_ptr = std::unique_ptr<T, Deleter>;


// template<typename T>
// void platform_allocator_deleter (T* ptr) {
// ptr->~T();
//   kphp::memory::platform_allocator<T>{}.deallocate(ptr, sizeof(T)); 
// }

template<typename T>
struct platform_allocator_deleter_t {
  void operator()(T* ptr) {
    if (ptr) {
      ptr->~T();
      kphp::memory::platform_allocator<T>{}.deallocate(ptr, sizeof(T));
    }
  }
};

template<typename T>
using unique_ptr_platform = unique_ptr<T, platform_allocator_deleter_t<T>>;

template<typename T, typename... Args>
auto make_unique(Args&&... args) {

  T* mem = kphp::memory::platform_allocator<T>{}.allocate(1);
  new (mem) T(std::forward<Args>(args)...);

  return kphp_ml::stl::unique_ptr_platform<T>(mem);
}

} // namespace kphp_ml::stl

template<>
struct std::hash<kphp::stl::string<kphp::memory::platform_allocator>> {
  std::size_t operator()(const kphp::stl::string<kphp::memory::platform_allocator>& s) const noexcept {
    std::string_view view{s.c_str(), s.size()};
    return std::hash<std::string_view>{}(view);
  }
};
