// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "runtime-common/core/runtime-core.h"

namespace zlib {

inline constexpr int64_t ENCODING_RAW = -0x0f;
inline constexpr int64_t ENCODING_DEFLATE = 0x0f;
inline constexpr int64_t ENCODING_GZIP = 0x1f;

inline constexpr int64_t MIN_COMPRESSION_LEVEL = -1;
inline constexpr int64_t MAX_COMPRESSION_LEVEL = 9;
inline constexpr int64_t DEFAULT_COMPRESSION_LEVEL = 6;

Optional<string> zlib_encode(std::span<const char> data, int64_t level, int64_t encoding) noexcept;

Optional<string> zlib_decode(std::span<const char> data, int64_t encoding) noexcept;

} // namespace zlib

inline string f$gzcompress(const string &data, int64_t level = zlib::MIN_COMPRESSION_LEVEL) noexcept {
  Optional<string> result{zlib::zlib_encode({data.c_str(), static_cast<size_t>(data.size())},
                                            level == zlib::MIN_COMPRESSION_LEVEL ? zlib::DEFAULT_COMPRESSION_LEVEL : level, zlib::ENCODING_DEFLATE)};
  return result.has_value() ? result.val() : string{};
}

inline string f$gzuncompress(const string &data) noexcept {
  Optional<string> result{zlib::zlib_decode({data.c_str(), static_cast<size_t>(data.size())}, zlib::ENCODING_DEFLATE)};
  return result.has_value() ? result.val() : string{};
}
