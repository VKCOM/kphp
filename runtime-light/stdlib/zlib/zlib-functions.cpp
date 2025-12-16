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
  if (address == nullptr) [[unlikely]] {
    return;
  }
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

  const auto deflate_res{deflate(std::addressof(zstrm), Z_FINISH)};
  if (deflate_res != Z_STREAM_END) [[unlikely]] {
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
  const auto inflate_res{inflate(std::addressof(zstrm), Z_NO_FLUSH)};

  if (inflate_res != Z_STREAM_END) [[unlikely]] {
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
  auto extract_int_option{[](int32_t lbound, int32_t ubound, const array_iterator<const mixed>& option, int32_t& dst) noexcept {
    const mixed& value = option.get_value();
    if (value.is_int() && value.as_int() >= lbound && value.as_int() <= ubound) {
      dst = value.as_int();
      return true;
    } else {
      kphp::log::warning("deflate_init() : option {} should be number between {}..{}", option.get_string_key().c_str(), lbound, ubound);
      return false;
    }
  }};
  switch (encoding) {
  case kphp::zlib::ENCODING_RAW:
  case kphp::zlib::ENCODING_DEFLATE:
  case kphp::zlib::ENCODING_GZIP:
    break;
  default:
    kphp::log::warning("deflate_init() : encoding should be one of ZLIB_ENCODING_RAW, ZLIB_ENCODING_DEFLATE, ZLIB_ENCODING_GZIP");
    return {};
  }
  for (const auto& option : options) {
    mixed value;
    if (!option.is_string_key()) {
      kphp::log::warning("deflate_init() : unsupported option");
      return {};
    }
    if (option.get_string_key() == string("level")) {
      if (!extract_int_option(-1, 9, option, level)) {
        return {};
      }
    } else if (option.get_string_key() == string("memory")) {
      if (!extract_int_option(1, 9, option, memory)) {
        return {};
      }
    } else if (option.get_string_key() == string("window")) {
      if (!extract_int_option(8, 15, option, window)) {
        return {};
      }
    } else if (option.get_string_key() == string("strategy")) {
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
          kphp::log::warning(
              "deflate_init() : option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
          return {};
        }
      } else {
        kphp::log::warning("deflate_init() : option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
        return {};
      }
    } else if (option.get_string_key() == string("dictionary")) {
      kphp::log::warning("deflate_init() : option dictionary isn't supported yet");
      return {};
    } else {
      kphp::log::warning("deflate_init() : unknown option name \"{}\"", option.get_string_key().c_str());
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

  auto err{deflateInit2(stream, level, Z_DEFLATED, encoding, memory, strategy)};
  if (err != Z_OK) {
    kphp::log::warning("deflate_init() : zlib error {}", zError(err));
    context.destroy();
    return {};
  }
  return context;
}
