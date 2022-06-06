// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

namespace vk::msgpack {
struct CheckInstanceDepth {
  static size_t depth;
  static constexpr size_t max_depth = 20;

  CheckInstanceDepth() {
    depth++;
  }

  CheckInstanceDepth(const CheckInstanceDepth &) = delete;
  CheckInstanceDepth &operator=(const CheckInstanceDepth &) = delete;

  static bool is_exceeded() {
    return depth > max_depth;
  }

  ~CheckInstanceDepth() {
    if (!is_exceeded()) {
      depth--;
    }
  }
};

} // namespace vk::msgpack
