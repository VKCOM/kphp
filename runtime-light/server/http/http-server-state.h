// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

enum class HttpMethod : uint8_t { GET, POST, HEAD, OTHER };

enum class HttpConnectionKind : uint8_t { KeepAlive, Close };

struct HttpServerInstanceState final : private vk::not_copyable {
  static constexpr auto ENCODING_GZIP = static_cast<uint32_t>(1U << 0U);
  static constexpr auto ENCODING_DEFLATE = static_cast<uint32_t>(1U << 1U);

  uint32_t encoding{};
  HttpMethod method{HttpMethod::OTHER};
  HttpConnectionKind connection_kind{HttpConnectionKind::Close};
  std::optional<string> raw_post_data;

  static HttpServerInstanceState &get() noexcept;
};
