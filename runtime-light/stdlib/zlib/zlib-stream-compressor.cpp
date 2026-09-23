// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/zlib/zlib-stream-compressor.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/zlib/zlib-functions.h"

namespace kphp::zlib {

voidpf stream_compressor::dynamic_calloc([[maybe_unused]] voidpf opaque, uInt items, uInt size) noexcept {
  auto* mem{kphp::memory::script::calloc(items, size)};
  if (mem == nullptr) [[unlikely]] {
    kphp::log::warning("zlib dynamic calloc: can't allocate {} bytes", items * size);
  }
  return mem;
}

void stream_compressor::dynamic_free([[maybe_unused]] voidpf opaque, voidpf address) noexcept {
  kphp::memory::script::free(address);
}

bool stream_compressor::init_if_needed(int32_t level, int32_t encoding, int32_t memory, int32_t strategy) noexcept {
  if (this->m_stream.has_value()) {
    return true;
  }

  if (level < MIN_COMPRESSION_LEVEL || level > MAX_COMPRESSION_LEVEL) [[unlikely]] {
    kphp::log::warning("incorrect compression level: {}", level);
    return false;
  }
  if (encoding != ENCODING_RAW && encoding != ENCODING_DEFLATE && encoding != ENCODING_GZIP) [[unlikely]] {
    kphp::log::warning("incorrect encoding: {}", encoding);
    return false;
  }

  this->m_stream.emplace(z_stream{.zalloc = dynamic_calloc, .zfree = dynamic_free, .opaque = nullptr});
  if (auto err{deflateInit2(std::addressof(*this->m_stream), level, Z_DEFLATED, encoding, memory, strategy)}; err != Z_OK) [[unlikely]] {
    kphp::log::warning("can't initialize zlib stream compressor: error {}", err);
    this->m_stream.reset();
    return false;
  }
  return true;
}

std::optional<string> stream_compressor::compress(std::span<const char> data, bool finish) noexcept {
  kphp::log::assertion(this->m_stream.has_value());
  z_stream& zstrm{*this->m_stream};

  // Extra headroom for Z_SYNC_FLUSH; the buffer still grows if this estimate is insufficient.
  static constexpr uint64_t EXTRA_OUT_SIZE{30};
  auto out_size{static_cast<uint64_t>(deflateBound(std::addressof(zstrm), data.size())) + EXTRA_OUT_SIZE};
  string out{static_cast<string::size_type>(out_size), false};

  // zlib advances next_in but does not modify input bytes; its API lacks const without ZLIB_CONST.
  zstrm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
  zstrm.avail_in = static_cast<uInt>(data.size());
  zstrm.next_out = reinterpret_cast<Bytef*>(out.buffer());
  zstrm.avail_out = static_cast<uInt>(out_size);

  const auto zlib_flush_mode{finish ? Z_FINISH : Z_SYNC_FLUSH};
  auto status{Z_OK};
  uint64_t buffer_used{};

  // If the output buffer is too small, grow it and continue writing compressed data.
  do {
    if (zstrm.avail_out == 0) {
      const auto previous_size{out_size};
      out_size += previous_size;
      out.reserve_at_least(static_cast<string::size_type>(out_size));
      zstrm.avail_out = static_cast<uInt>(out_size - previous_size);
      zstrm.next_out = reinterpret_cast<Bytef*>(std::next(out.buffer(), static_cast<ptrdiff_t>(buffer_used)));
    }
    status = deflate(std::addressof(zstrm), zlib_flush_mode);
    buffer_used = out_size - zstrm.avail_out;
  } while (status == Z_OK && zstrm.avail_out == 0);

  // Repeated empty Z_SYNC_FLUSH calls have no output and may return Z_BUF_ERROR.
  if (status == Z_BUF_ERROR && !finish && data.empty() && buffer_used == 0) {
    return string{};
  }
  if ((finish && status != Z_STREAM_END) || (!finish && status != Z_OK)) [[unlikely]] {
    kphp::log::warning("zlib error while incrementally compressing data: {}", status);
    return {};
  }

  out.shrink(static_cast<string::size_type>(buffer_used));
  return out;
}

void stream_compressor::close() noexcept {
  if (this->m_stream.has_value()) {
    deflateEnd(std::addressof(*this->m_stream));
    this->m_stream.reset();
  }
}

stream_compressor::~stream_compressor() {
  close();
}

} // namespace kphp::zlib
