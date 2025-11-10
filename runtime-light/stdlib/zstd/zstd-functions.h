// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "runtime-common/core/runtime-core.h"

namespace kphp::zstd {

inline constexpr int64_t DEFAULT_COMPRESS_LEVEL = 3;

std::optional<string> compress(std::span<const std::byte> data, int64_t level = DEFAULT_COMPRESS_LEVEL,
                               std::span<const std::byte> dict = std::span<const std::byte>{}) noexcept;

std::optional<string> uncompress(std::span<const std::byte> data, std::span<const std::byte> dict = std::span<const std::byte>{}) noexcept;

} // namespace kphp::zstd

inline Optional<string> f$zstd_compress(const string& data, int64_t level = kphp::zstd::DEFAULT_COMPRESS_LEVEL) noexcept {
  auto res{kphp::zstd::compress({reinterpret_cast<const std::byte*>(data.c_str()), static_cast<size_t>(data.size())}, level)};
  if (!res) [[unlikely]] {
    return false;
  }
  return res.value();
}

inline Optional<string> f$zstd_uncompress(const string& data) noexcept {
  auto res{kphp::zstd::uncompress({reinterpret_cast<const std::byte*>(data.c_str()), static_cast<size_t>(data.size())})};
  if (!res) [[unlikely]] {
    return false;
  }
  return res.value();
}

inline Optional<string> f$zstd_compress_dict(const string& data, const string& dict) noexcept {
  auto res{kphp::zstd::compress({reinterpret_cast<const std::byte*>(data.c_str()), static_cast<size_t>(data.size())}, kphp::zstd::DEFAULT_COMPRESS_LEVEL,
                                {reinterpret_cast<const std::byte*>(dict.c_str()), static_cast<size_t>(dict.size())})};
  if (!res) [[unlikely]] {
    return false;
  }
  return res.value();
}

inline Optional<string> f$zstd_uncompress_dict(const string& data, const string& dict) noexcept {
  auto res{kphp::zstd::uncompress({reinterpret_cast<const std::byte*>(data.c_str()), static_cast<size_t>(data.size())},
                                  {reinterpret_cast<const std::byte*>(dict.c_str()), static_cast<size_t>(dict.size())})};
  if (!res) [[unlikely]] {
    return false;
  }
  return res.value();
}
