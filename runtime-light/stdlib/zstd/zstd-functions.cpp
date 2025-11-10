// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/zstd/zstd-functions.h"

#include <cstddef>
#include <cstdint>
#include <span>

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd/zstd.h"

#include "common/containers/final_action.h"
#include "common/smart_ptrs/unique_ptr_with_delete_function.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {

static_assert(2 * ZSTD_BLOCKSIZE_MAX < StringLibContext::STATIC_BUFFER_LENGTH, "double block size is expected to be less then buffer size");

constexpr ZSTD_customMem zstd_allocator{[](void*, size_t size) { return kphp::memory::script::alloc(size); },
                                        [](void*, void* ptr) { return kphp::memory::script::free(ptr); }};

} // namespace

namespace kphp::zstd {

Optional<string> compress(std::span<const std::byte> data, int64_t level, std::span<const std::byte> dict) noexcept {
  const int32_t min_level{ZSTD_minCLevel()};
  const int32_t max_level{ZSTD_maxCLevel()};
  if (level < min_level || max_level < level) {
    kphp::log::warning("zstd_compress: compression level ({}) must be within [{}..{}]", level, min_level, max_level);
    return false;
  }

  ZSTD_CCtx* ctx{ZSTD_createCCtx_advanced(zstd_allocator)};
  if (!ctx) {
    kphp::log::warning("zstd_compress: can not create context");
    return false;
  }
  const auto finalizer{vk::finally([&ctx]() noexcept { ZSTD_freeCCtx(ctx); })};

  size_t result{ZSTD_CCtx_setParameter(ctx, ZSTD_c_compressionLevel, static_cast<int>(level))};
  if (ZSTD_isError(result)) {
    kphp::log::warning("zstd_compress: can not init context: {}", ZSTD_getErrorName(result));
    return false;
  }

  result = ZSTD_CCtx_loadDictionary_byReference(ctx, dict.data(), dict.size());
  if (ZSTD_isError(result)) {
    kphp::log::warning("zstd_compress: can not load dict: {}", ZSTD_getErrorName(result));
    return false;
  }

  kphp::log::assertion(ZSTD_CStreamOutSize() <= StringLibContext::STATIC_BUFFER_LENGTH);
  ZSTD_outBuffer out{StringLibContext::get().static_buf.get(), StringLibContext::STATIC_BUFFER_LENGTH, 0};
  ZSTD_inBuffer in{data.data(), data.size(), 0};

  string encoded_string{};
  do {
    result = ZSTD_compressStream2(ctx, &out, &in, ZSTD_e_end);
    if (ZSTD_isError(result)) {
      kphp::log::warning("zstd_compress: got zstd stream compression error: {}", ZSTD_getErrorName(result));
      return false;
    }
    encoded_string.append(static_cast<char*>(out.dst), out.pos);
    out.pos = 0;
  } while (result);
  return encoded_string;
}

Optional<string> uncompress(std::span<const std::byte> data, std::span<const std::byte> dict) noexcept {
  auto size{ZSTD_getFrameContentSize(data.data(), data.size())};
  if (size == ZSTD_CONTENTSIZE_ERROR) {
    kphp::log::warning("zstd_uncompress: it was not compressed by zstd");
    return false;
  }

  ZSTD_DCtx* ctx{ZSTD_createDCtx_advanced(zstd_allocator)};
  if (!ctx) {
    kphp::log::warning("zstd_uncompress: can not create context");
    return false;
  }
  const auto finalizer{vk::finally([&ctx]() noexcept { ZSTD_freeDCtx(ctx); })};

  size_t result{ZSTD_DCtx_loadDictionary_byReference(ctx, dict.data(), dict.size())};
  if (ZSTD_isError(result)) {
    kphp::log::warning("zstd_uncompress: can not load dict: {}", ZSTD_getErrorName(result));
    return false;
  }

  if (size != ZSTD_CONTENTSIZE_UNKNOWN) {
    if (size > string::max_size()) {
      kphp::log::warning("zstd_uncompress: trying to uncompress too large data");
      return false;
    }
    string decompressed{static_cast<string::size_type>(size), false};
    result = ZSTD_decompressDCtx(ctx, decompressed.buffer(), size, data.data(), data.size());
    if (ZSTD_isError(result)) {
      kphp::log::warning("zstd_uncompress: got zstd error: {}", ZSTD_getErrorName(result));
      return false;
    }
    return decompressed;
  }

  if (ZSTD_isError(result)) {
    kphp::log::warning("zstd_uncompress: can not init stream: {}", ZSTD_getErrorName(result));
    return false;
  }

  kphp::log::assertion(ZSTD_DStreamOutSize() <= StringLibContext::STATIC_BUFFER_LENGTH);
  ZSTD_inBuffer in{data.data(), data.size(), 0};
  ZSTD_outBuffer out{StringLibContext::get().static_buf.get(), StringLibContext::STATIC_BUFFER_LENGTH, 0};

  string decoded_string{};
  while (in.pos < in.size) {
    if (out.pos == out.size) {
      decoded_string.append(static_cast<char*>(out.dst), static_cast<string::size_type>(out.pos));
      out.pos = 0;
    }

    result = ZSTD_decompressStream(ctx, &out, &in);
    if (ZSTD_isError(result)) {
      kphp::log::warning("zstd_uncompress: can not decompress stream: {}", ZSTD_getErrorName(result));
      return false;
    }
    if (result == 0) {
      break;
    }
  }
  decoded_string.append(static_cast<char*>(out.dst), static_cast<string::size_type>(out.pos));
  return decoded_string;
}

} // namespace kphp::zstd
