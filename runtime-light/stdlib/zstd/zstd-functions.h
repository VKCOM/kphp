// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

#define ZSTD_STATIC_LINKING_ONLY

#include "zstd/zstd.h"

constexpr int DEFAULT_COMPRESS_LEVEL = 3;

namespace zstd_impl_ {

Optional<string> compress(const string& data, int64_t level = DEFAULT_COMPRESS_LEVEL, const string& dict = string{}) noexcept;

Optional<string> uncompress(const string& data, const string& dict = string{}) noexcept;

} // namespace zstd_impl_

inline Optional<string> f$zstd_compress(const string& data, int64_t level = DEFAULT_COMPRESS_LEVEL) noexcept {
  const int32_t min_level{ZSTD_minCLevel()};
  const int32_t max_level{ZSTD_maxCLevel()};
  if (level < min_level || max_level < level) {
    kphp::log::warning("zstd_compress: compression level ({}) must be within [{}..{}] or equal to 0", level, min_level, max_level);
    return false;
  }

  return zstd_impl_::compress(data, level);
}

inline Optional<string> f$zstd_uncompress(const string& data) noexcept {
  return zstd_impl_::uncompress(data);
}

inline Optional<string> f$zstd_compress_dict(const string& data, const string& dict) noexcept {
  return zstd_impl_::compress(data, DEFAULT_COMPRESS_LEVEL, dict);
}

inline Optional<string> f$zstd_uncompress_dict(const string& data, const string& dict) noexcept {
  return zstd_impl_::uncompress(data, dict);
}
