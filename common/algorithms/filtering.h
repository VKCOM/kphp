#pragma once

#include <algorithm>

namespace vk {

template<typename C, typename P>
void erase_if(C &container, P &&predicate) {
  auto start = std::remove_if(std::begin(container), std::end(container), std::forward<P>(predicate));
  container.erase(start, container.end());
}

} // namespace vk
