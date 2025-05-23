//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace kphp::diagnostic {

std::size_t get_async_stacktrace(std::span<void*> addresses);

} // namespace kphp::diagnostic
