// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/zlib/zlib-functions.h"

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <zconf.h>
#include <zlib.h>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/string/string-state.h"

namespace {

voidpf zlib_static_alloc(voidpf opaque, uInt items, uInt size) noexcept {
  auto *buf_pos_ptr{reinterpret_cast<size_t *>(opaque)};
  auto required_mem{static_cast<size_t>(items * size)};
  if (items == 0 || StringInstanceState::STATIC_BUFFER_LENGTH - *buf_pos_ptr < required_mem) [[unlikely]] {
    php_warning("zlib static alloc: can't allocate %zu bytes", required_mem);
    return Z_NULL;
  }

  auto *mem{StringInstanceState::get().static_buf.data()};
  std::advance(mem, *buf_pos_ptr);
  *buf_pos_ptr += required_mem;
  return mem;
}

void zlib_static_free(voidpf /*unused*/, voidpf /*unused*/) noexcept {}

// FIXME: we can use script allocator instead of k2:: routines
voidpf zlib_dynamic_alloc(voidpf /*opaque*/, uInt items, uInt size) noexcept {
  auto required_mem{static_cast<size_t>(items) * size};
  auto *mem{k2::alloc(required_mem)};
  if (mem == nullptr) [[unlikely]] {
    php_warning("zlib dynamic alloc: can't allocate %zu bytes", required_mem);
    return Z_NULL;
  }
  return mem;
}

void zlib_dynamic_free(voidpf /*opaque*/, voidpf address) noexcept {
  k2::free(address);
}

} // namespace

namespace zlib_impl_ {

Optional<string> zlib_encode(std::span<const char> data, int64_t level, int64_t encoding) noexcept {
  if (level < zlib_impl_::ZLIB_MIN_COMPRESSION_LEVEL || level > zlib_impl_::ZLIB_MAX_COMPRESSION_LEVEL) [[unlikely]] {
    php_warning("incorrect compression level: %" PRIi64, level);
    return false;
  }
  if (encoding != ZLIB_ENCODING_RAW && encoding != ZLIB_ENCODING_DEFLATE && encoding != ZLIB_ENCODING_GZIP) [[unlikely]] {
    php_warning("incorrect encoding: %" PRIi64, encoding);
    return false;
  }

  z_stream zstrm{};
  size_t buf_pos{};
  zstrm.zalloc = zlib_static_alloc;
  zstrm.zfree = zlib_static_free;
  zstrm.opaque = std::addressof(buf_pos);

  if (deflateInit2(std::addressof(zstrm), level, Z_DEFLATED, encoding, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) [[unlikely]] {
    php_warning("can't initialize zlib encode for data of length %zu", data.size());
    return false;
  }

  auto &runtime_ctx{RuntimeContext::get()};
  auto out_size_upper_bound{static_cast<size_t>(compressBound(data.size()))};
  runtime_ctx.static_SB.clean().reserve(out_size_upper_bound);

  zstrm.avail_in = data.size();
  zstrm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
  zstrm.avail_out = out_size_upper_bound;
  zstrm.next_out = reinterpret_cast<Bytef *>(runtime_ctx.static_SB.buffer());

  const auto deflate_res{deflate(std::addressof(zstrm), Z_FINISH)};
  deflateEnd(std::addressof(zstrm));
  if (deflate_res != Z_STREAM_END) [[unlikely]] {
    php_warning("can't encode data of length %zu due to zlib error %d", data.size(), deflate_res);
    return false;
  }

  runtime_ctx.static_SB.set_pos(static_cast<int64_t>(zstrm.total_out));
  return runtime_ctx.static_SB.str();
}

Optional<string> zlib_decode(std::span<const char> data, int64_t encoding) noexcept {
  z_stream zstrm{};
  zstrm.zalloc = zlib_dynamic_alloc;
  zstrm.zfree = zlib_dynamic_free;
  zstrm.opaque = nullptr;
  zstrm.avail_in = data.size();
  zstrm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));

  if (inflateInit2(std::addressof(zstrm), encoding) != Z_OK) [[unlikely]] {
    php_warning("can't initialize zlib decode for data of length %zu", data.size());
    return false;
  }

  auto &string_st{StringInstanceState::get()};
  zstrm.avail_out = StringInstanceState::STATIC_BUFFER_LENGTH;
  zstrm.next_out = reinterpret_cast<Bytef *>(string_st.static_buf.data());
  const auto inflate_res{inflate(std::addressof(zstrm), Z_NO_FLUSH)};
  inflateEnd(std::addressof(zstrm));

  if (inflate_res != Z_STREAM_END) [[unlikely]] {
    php_warning("can't decode data of length %zu due to zlib error %d", data.size(), inflate_res);
    return false;
  }

  return string{string_st.static_buf.data(), StringInstanceState::STATIC_BUFFER_LENGTH - zstrm.avail_out};
}

} // namespace zlib_impl_
