// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

namespace kphp::coro {

enum class poll_op : uint8_t {
  read,
  write,
};

enum class poll_status : uint8_t {
  event,
  timeout,
  error,
  closed,
};

} // namespace kphp::coro
