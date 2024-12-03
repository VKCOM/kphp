// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "runtime-common/core/runtime-core.h"

inline constexpr int64_t ZLIB_ENCODING_RAW = -0x0f;
inline constexpr int64_t ZLIB_ENCODING_DEFLATE = 0x0f;
inline constexpr int64_t ZLIB_ENCODING_GZIP = 0x1f;

namespace zlib_impl_ {

inline constexpr int64_t ZLIB_MIN_COMPRESSION_LEVEL = -1;
inline constexpr int64_t ZLIB_MAX_COMPRESSION_LEVEL = 9;
inline constexpr int64_t ZLIB_DEFAULT_COMPRESSION_LEVEL = 6;

Optional<string> zlib_encode(std::span<const char> data, int64_t level, int64_t encoding) noexcept;

Optional<string> zlib_decode(std::span<const char> data, int64_t encoding) noexcept;

} // namespace zlib_impl_

inline string f$gzcompress(const string &data, int64_t level = zlib_impl_::ZLIB_MIN_COMPRESSION_LEVEL) noexcept {
  Optional<string> result{zlib_impl_::zlib_encode({data.c_str(), static_cast<size_t>(data.size())},
                                                  level == zlib_impl_::ZLIB_MIN_COMPRESSION_LEVEL ? zlib_impl_::ZLIB_DEFAULT_COMPRESSION_LEVEL : level,
                                                  ZLIB_ENCODING_DEFLATE)};
  if (result.has_value()) {
    return result.val();
  }
  return string("");
}

inline string f$gzuncompress(const string &data) noexcept {
  Optional<string> result{zlib_impl_::zlib_decode({data.c_str(), static_cast<size_t>(data.size())}, ZLIB_ENCODING_DEFLATE)};
  if (result.has_value()) {
    return result.val();
  }
  return string("");
}
