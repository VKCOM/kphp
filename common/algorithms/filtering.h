// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>

namespace vk {

template<typename C, typename P>
void erase_if(C& container, P&& predicate) {
  auto start = std::remove_if(std::begin(container), std::end(container), std::forward<P>(predicate));
  container.erase(start, container.end());
}

} // namespace vk
