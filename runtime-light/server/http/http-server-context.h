// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"

struct HttpServerComponentContext final : private vk::not_copyable {
  static constexpr auto ENCODING_GZIP = static_cast<uint32_t>(1U << 0U);
  static constexpr auto ENCODING_DEFLATE = static_cast<uint32_t>(1U << 1U);

  uint32_t encoding{};

  static HttpServerComponentContext &get() noexcept;
};
