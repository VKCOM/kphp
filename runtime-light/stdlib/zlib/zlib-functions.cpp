// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/zlib/zlib-functions.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>

#include "zlib/zconf.h"
#include "zlib/zlib.h"

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/string-state.h"

namespace {

voidpf zlib_static_alloc(voidpf opaque, uInt items, uInt size) noexcept {
  auto* buf_pos_ptr{reinterpret_cast<size_t*>(opaque)};
  auto required_mem{static_cast<size_t>(items * size)};
  if (items == 0 || StringInstanceState::STATIC_BUFFER_LENGTH - *buf_pos_ptr < required_mem) [[unlikely]] {
    kphp::log::warning("zlib static alloc: can't allocate {} bytes", required_mem);
    return Z_NULL;
  }

  auto* mem{std::next(StringInstanceState::get().static_buf.get(), *buf_pos_ptr)};
  *buf_pos_ptr += required_mem;
  return mem;
}

void zlib_static_free([[maybe_unused]] voidpf opaque, [[maybe_unused]] voidpf address) noexcept {}

voidpf zlib_dynamic_alloc([[maybe_unused]] voidpf opaque, uInt items, uInt size) noexcept {
  auto* mem{kphp::memory::script::calloc(items, size)};
  if (mem == nullptr) [[unlikely]] {
    kphp::log::warning("zlib dynamic alloc: can't allocate {} bytes", items * size);
  }
  return mem;
}

void zlib_dynamic_free([[maybe_unused]] voidpf opaque, voidpf address) noexcept {
  kphp::memory::script::free(address);
}

} // namespace

namespace kphp::zlib {

std::optional<string> encode(std::span<const char> data, int64_t level, int64_t encoding) noexcept {
  if (level < MIN_COMPRESSION_LEVEL || level > MAX_COMPRESSION_LEVEL) [[unlikely]] {
    kphp::log::warning("incorrect compression level: {}", level);
    return {};
  }
  if (encoding != ENCODING_RAW && encoding != ENCODING_DEFLATE && encoding != ENCODING_GZIP) [[unlikely]] {
    kphp::log::warning("incorrect encoding: {}", encoding);
    return {};
  }

  z_stream zstrm{};
  // always clear zlib's state
  const auto finalizer{vk::finally([&zstrm]() noexcept { deflateEnd(std::addressof(zstrm)); })};

  size_t buf_pos{};
  zstrm.zalloc = zlib_static_alloc;
  zstrm.zfree = zlib_static_free;
  zstrm.opaque = std::addressof(buf_pos);

  if (deflateInit2(std::addressof(zstrm), level, Z_DEFLATED, encoding, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) [[unlikely]] {
    kphp::log::warning("can't initialize zlib encode for data of length {}", data.size());
    return {};
  }

  auto& runtime_ctx{RuntimeContext::get()};
  auto out_size_upper_bound{static_cast<size_t>(deflateBound(std::addressof(zstrm), data.size()))};
  runtime_ctx.static_SB.clean().reserve(out_size_upper_bound);

  zstrm.avail_in = data.size();
  zstrm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
  zstrm.avail_out = out_size_upper_bound;
  zstrm.next_out = reinterpret_cast<Bytef*>(runtime_ctx.static_SB.buffer());

  if (const auto deflate_res{deflate(std::addressof(zstrm), Z_FINISH)}; deflate_res != Z_STREAM_END) [[unlikely]] {
    kphp::log::warning("can't encode data of length {} due to zlib error {}", data.size(), deflate_res);
    return {};
  }

  runtime_ctx.static_SB.set_pos(static_cast<int64_t>(zstrm.total_out));
  return runtime_ctx.static_SB.str();
}

std::optional<string> decode(std::span<const char> data, int64_t encoding) noexcept {
  z_stream zstrm{};
  // always clear zlib's state
  const auto finalizer{vk::finally([&zstrm]() noexcept { inflateEnd(std::addressof(zstrm)); })};

  size_t buf_pos{};
  zstrm.zalloc = zlib_static_alloc;
  zstrm.zfree = zlib_static_free;
  zstrm.opaque = std::addressof(buf_pos);
  zstrm.avail_in = data.size();
  zstrm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

  if (inflateInit2(std::addressof(zstrm), encoding) != Z_OK) [[unlikely]] {
    kphp::log::warning("can't initialize zlib decode for data of length {}", data.size());
    return {};
  }

  auto& runtime_ctx{RuntimeContext::get()};
  runtime_ctx.static_SB.clean().reserve(StringInstanceState::STATIC_BUFFER_LENGTH);
  zstrm.avail_out = StringInstanceState::STATIC_BUFFER_LENGTH;
  zstrm.next_out = reinterpret_cast<Bytef*>(runtime_ctx.static_SB.buffer());

  if (const auto inflate_res{inflate(std::addressof(zstrm), Z_NO_FLUSH)}; inflate_res != Z_STREAM_END) [[unlikely]] {
    kphp::log::warning("can't decode data of length {} due to zlib error {}", data.size(), inflate_res);
    return {};
  }

  return string{runtime_ctx.static_SB.buffer(), StringInstanceState::STATIC_BUFFER_LENGTH - zstrm.avail_out};
}

} // namespace kphp::zlib

class_instance<C$DeflateContext> f$deflate_init(int64_t encoding, const array<mixed>& options) noexcept {
  int32_t level{-1};
  int32_t memory{8};
  int32_t window{15};
  auto strategy{Z_DEFAULT_STRATEGY};
  constexpr auto extract_int_option{[](int32_t lbound, int32_t ubound, const array_iterator<const mixed>& option, int32_t& dst) noexcept {
    if (const mixed & value{option.get_value()}; value.is_int() && value.as_int() >= lbound && value.as_int() <= ubound) {
      dst = value.as_int();
      return true;
    } else {
      kphp::log::warning("option {} should be number between {}..{}", option.get_string_key().c_str(), lbound, ubound);
      return false;
    }
  }};
  switch (encoding) {
  case kphp::zlib::ENCODING_RAW:
  case kphp::zlib::ENCODING_DEFLATE:
  case kphp::zlib::ENCODING_GZIP:
    break;
  default:
    kphp::log::warning("encoding should be one of ZLIB_ENCODING_RAW, ZLIB_ENCODING_DEFLATE, ZLIB_ENCODING_GZIP");
    return {};
  }
  for (const auto& option : options) {
    mixed value;
    if (!option.is_string_key()) {
      kphp::log::warning("unsupported option");
      return {};
    }
    const auto& key = option.get_string_key();
    if (std::string_view{key.c_str(), key.size()} == "level") {
      if (!extract_int_option(-1, 9, option, level)) {
        return {};
      }
    } else if (std::string_view{key.c_str(), key.size()} == "memory") {
      if (!extract_int_option(1, 9, option, memory)) {
        return {};
      }
    } else if (std::string_view{key.c_str(), key.size()} == "window") {
      if (!extract_int_option(8, 15, option, window)) {
        return {};
      }
    } else if (std::string_view{key.c_str(), key.size()} == "strategy") {
      value = option.get_value();
      if (value.is_int()) {
        switch (value.as_int()) {
        case Z_FILTERED:
        case Z_HUFFMAN_ONLY:
        case Z_RLE:
        case Z_FIXED:
        case Z_DEFAULT_STRATEGY:
          strategy = value.as_int();
          break;
        default:
          kphp::log::warning("option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
          return {};
        }
      } else {
        kphp::log::warning("option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
        return {};
      }
    } else if (std::string_view{key.c_str(), key.size()} == "dictionary") {
      kphp::log::warning("option dictionary isn't supported yet");
      return {};
    } else {
      kphp::log::warning("unknown option name \"{}\"", option.get_string_key().c_str());
      return {};
    }
  }

  class_instance<C$DeflateContext> context;
  context.alloc();

  z_stream* stream{std::addressof(context.get()->stream)};
  stream->zalloc = zlib_dynamic_alloc;
  stream->zfree = zlib_dynamic_free;
  stream->opaque = nullptr;

  if (encoding < 0) {
    encoding += 15 - window;
  } else {
    encoding -= 15 - window;
  }

  if (auto err{deflateInit2(stream, level, Z_DEFLATED, encoding, memory, strategy)}; err != Z_OK) {
    kphp::log::warning("zlib error {}", zError(err));
    context.destroy();
    return {};
  }
  return context;
}

Optional<string> f$deflate_add(const class_instance<C$DeflateContext>& context, const string& data, int64_t flush_type) noexcept {
  switch (flush_type) {
  case Z_BLOCK:
  case Z_NO_FLUSH:
  case Z_PARTIAL_FLUSH:
  case Z_SYNC_FLUSH:
  case Z_FULL_FLUSH:
  case Z_FINISH:
    break;
  default:
    kphp::log::warning("flush type should be one of ZLIB_NO_FLUSH, ZLIB_PARTIAL_FLUSH, ZLIB_SYNC_FLUSH, ZLIB_FULL_FLUSH, ZLIB_FINISH, ZLIB_BLOCK, ZLIB_TREES");
    return {};
  }

  z_stream* stream{std::addressof(context.get()->stream)};
  auto out_size{deflateBound(stream, data.size()) + 30};
  out_size = out_size < 64 ? 64 : out_size;
  string out{static_cast<string::size_type>(out_size), false};
  stream->next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.c_str()));
  stream->next_out = reinterpret_cast<Bytef*>(out.buffer());
  stream->avail_in = data.size();
  stream->avail_out = out_size;

  auto status{Z_OK};
  uint64_t buffer_used{};
  do {
    if (stream->avail_out == 0) {
      out.reserve_at_least(out_size + 64);
      out_size += 64;
      stream->avail_out = 64;
      stream->next_out = reinterpret_cast<Bytef*>(std::next(out.buffer(), buffer_used));
    }
    status = deflate(stream, flush_type);
    buffer_used = out_size - stream->avail_out;
  } while (status == Z_OK && stream->avail_out == 0);

  std::ptrdiff_t len{};
  switch (status) {
  case Z_OK:
    len = std::distance(reinterpret_cast<Bytef*>(out.buffer()), stream->next_out);
    out.shrink(len);
    return out;
  case Z_STREAM_END:
    len = std::distance(reinterpret_cast<Bytef*>(out.buffer()), stream->next_out);
    deflateReset(stream);
    out.shrink(len);
    return out;
  default:
    kphp::log::warning("zlib error {}", zError(status));
    return {};
  }
}
