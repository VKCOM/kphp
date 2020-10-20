#pragma once

#include <type_traits>

namespace vk {
struct identity {
  template<class T>
  constexpr T &&operator()(T &&t) const noexcept {
    return std::forward<T>(t);
  }
  using is_transparent = std::true_type;
};
}
