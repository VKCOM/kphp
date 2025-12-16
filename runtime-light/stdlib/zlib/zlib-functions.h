// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/zlib/deflatecontext.h"

namespace kphp::zlib {

inline constexpr int64_t ENCODING_RAW = -0x0f;
inline constexpr int64_t ENCODING_DEFLATE = 0x0f;
inline constexpr int64_t ENCODING_GZIP = 0x1f;

inline constexpr int64_t MIN_COMPRESSION_LEVEL = -1;
inline constexpr int64_t MAX_COMPRESSION_LEVEL = 9;
inline constexpr int64_t DEFAULT_COMPRESSION_LEVEL = 6;

std::optional<string> encode(std::span<const char> data, int64_t level, int64_t encoding) noexcept;

std::optional<string> decode(std::span<const char> data, int64_t encoding) noexcept;

} // namespace kphp::zlib

inline string f$gzcompress(const string& data, int64_t level = kphp::zlib::MIN_COMPRESSION_LEVEL) noexcept {
  level = level == kphp::zlib::MIN_COMPRESSION_LEVEL ? kphp::zlib::DEFAULT_COMPRESSION_LEVEL : level;
  return kphp::zlib::encode({data.c_str(), static_cast<size_t>(data.size())}, level, kphp::zlib::ENCODING_DEFLATE).value_or(string{});
}

inline string f$gzuncompress(const string& data) noexcept {
  return kphp::zlib::decode({data.c_str(), static_cast<size_t>(data.size())}, kphp::zlib::ENCODING_DEFLATE).value_or(string{});
}

inline string f$gzencode(const string& data, int64_t level = kphp::zlib::MIN_COMPRESSION_LEVEL) noexcept {
  level = level == kphp::zlib::MIN_COMPRESSION_LEVEL ? kphp::zlib::DEFAULT_COMPRESSION_LEVEL : level;
  return kphp::zlib::encode({data.c_str(), static_cast<size_t>(data.size())}, level, kphp::zlib::ENCODING_GZIP).value_or(string{});
}

inline string f$gzdecode(const string& data) noexcept {
  return kphp::zlib::decode({data.c_str(), static_cast<size_t>(data.size())}, kphp::zlib::ENCODING_GZIP).value_or(string{});
}

inline string f$gzdeflate(const string& data, int64_t level = kphp::zlib::MIN_COMPRESSION_LEVEL) noexcept {
  level = level == kphp::zlib::MIN_COMPRESSION_LEVEL ? kphp::zlib::DEFAULT_COMPRESSION_LEVEL : level;
  return kphp::zlib::encode({data.c_str(), static_cast<size_t>(data.size())}, level, kphp::zlib::ENCODING_RAW).value_or(string{});
}

inline string f$gzinflate(const string& data) noexcept {
  return kphp::zlib::decode({data.c_str(), static_cast<size_t>(data.size())}, kphp::zlib::ENCODING_RAW).value_or(string{});
}

class_instance<C$DeflateContext> f$deflate_init(int64_t encoding, const array<mixed>& options = {}) noexcept;

Optional<string> f$deflate_add(const class_instance<C$DeflateContext>& context, const string& data, int64_t flush_type = Z_SYNC_FLUSH) noexcept;
