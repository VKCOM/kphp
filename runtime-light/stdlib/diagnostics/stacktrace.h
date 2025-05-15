//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>

namespace kphp::diagnostic {

std::size_t get_async_stacktrace(void** data, std::size_t len);

template<std::size_t Len>
std::size_t get_async_stacktrace(std::array<void*, Len>& addresses) {
  return get_async_stacktrace(addresses.data(), addresses.size());
}

} // namespace kphp::diagnostic
