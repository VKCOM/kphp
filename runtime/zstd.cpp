// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#define ZSTD_STATIC_LINKING_ONLY

#include <zstd.h>

#include "common/smart_ptrs/unique_ptr_with_delete_function.h"

#include "runtime/string_functions.h"
#include "runtime/allocator.h"
#include "runtime/zstd.h"

namespace {

static_assert(2 * ZSTD_BLOCKSIZE_MAX < PHP_BUF_LEN, "double block size is expected to be less then buffer size");

ZSTD_customMem make_custom_alloc() noexcept {
  return ZSTD_customMem{
    [](void *, size_t size) { return dl::script_allocator_malloc(size); },
    [](void *, void *address) { dl::script_allocator_free(address); },
    nullptr
  };
}

template<class T, size_t (*Deleter)(T *)>
void free_ctx_wrapper(T *ptr) { Deleter(ptr); }

using ZSTD_CCtxPtr = vk::unique_ptr_with_delete_function<ZSTD_CStream, free_ctx_wrapper<ZSTD_CCtx, ZSTD_freeCCtx>>;
using ZSTD_DCtxPtr = vk::unique_ptr_with_delete_function<ZSTD_DCtx, free_ctx_wrapper<ZSTD_DCtx, ZSTD_freeDCtx>>;

Optional<string> zstd_compress_impl(const string &data, int64_t level = DEFAULT_COMPRESS_LEVEL, const string &dict = string{}) noexcept {
  ZSTD_CCtxPtr ctx{ZSTD_createCCtx_advanced(make_custom_alloc())};
  if (!ctx) {
    php_warning("zstd_compress: can not create context");
    return false;
  }

  size_t result = ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_compressionLevel, static_cast<int>(level));
  if (ZSTD_isError(result)) {
    php_warning("zstd_compress: can not init context: %s", ZSTD_getErrorName(result));
    return false;
  }

  result = ZSTD_CCtx_loadDictionary_byReference(ctx.get(), dict.c_str(), dict.size());
  if (ZSTD_isError(result)) {
    php_warning("zstd_compress: can not load dict: %s", ZSTD_getErrorName(result));
    return false;
  }

  php_assert(ZSTD_CStreamOutSize() <= PHP_BUF_LEN);
  ZSTD_outBuffer out{php_buf, PHP_BUF_LEN, 0};
  ZSTD_inBuffer in{data.c_str(), data.size(), 0};

  string encoded_string;
  do {
    result = ZSTD_compressStream2(ctx.get(), &out, &in, ZSTD_e_end);
    if (ZSTD_isError(result)) {
      php_warning("zstd_compress: got zstd stream compression error: %s", ZSTD_getErrorName(result));
      return false;
    }
    encoded_string.append(static_cast<char *>(out.dst), out.pos);
    out.pos = 0;
  } while (result);
  return encoded_string;
}

Optional<string> zstd_uncompress_impl(const string &data, const string &dict = string{}) noexcept {
  auto size = ZSTD_getFrameContentSize(data.c_str(), data.size());
  if (size == ZSTD_CONTENTSIZE_ERROR) {
    php_warning("zstd_uncompress: it was not compressed by zstd");
    return false;
  }

  ZSTD_DCtxPtr ctx{ZSTD_createDCtx_advanced(make_custom_alloc())};
  if (!ctx) {
    php_warning("zstd_uncompress: can not create context");
    return false;
  }

  size_t result = ZSTD_DCtx_loadDictionary_byReference(ctx.get(), dict.c_str(), dict.size());
  if (ZSTD_isError(result)) {
    php_warning("zstd_uncompress: can not load dict: %s", ZSTD_getErrorName(result));
    return false;
  }

  if (size != ZSTD_CONTENTSIZE_UNKNOWN) {
    if (size > string::max_size()) {
      php_warning("zstd_uncompress: trying to uncompress too large data");
      return false;
    }
    string decompressed{static_cast<string::size_type>(size), false};
    result = ZSTD_decompressDCtx(ctx.get(), decompressed.buffer(), size, data.c_str(), data.size());
    if (ZSTD_isError(result)) {
      php_warning("zstd_uncompress: got zstd error: %s", ZSTD_getErrorName(result));
      return false;
    }
    return decompressed;
  }

  if (ZSTD_isError(result)) {
    php_warning("zstd_uncompress: can not init stream: %s", ZSTD_getErrorName(result));
    return false;
  }

  php_assert(ZSTD_DStreamOutSize() <= PHP_BUF_LEN);
  ZSTD_inBuffer in{data.c_str(), data.size(), 0};
  ZSTD_outBuffer out{php_buf, PHP_BUF_LEN, 0};

  string decoded_string;
  while (in.pos < in.size) {
    if (out.pos == out.size) {
      decoded_string.append(static_cast<char *>(out.dst), static_cast<string::size_type>(out.pos));
      out.pos = 0;
    }

    result = ZSTD_decompressStream(ctx.get(), &out, &in);
    if (ZSTD_isError(result)) {
      php_warning("zstd_uncompress: can not decompress stream: %s", ZSTD_getErrorName(result));
      return false;
    }
    if (result == 0) {
      break;
    }
  }
  decoded_string.append(static_cast<char *>(out.dst), static_cast<string::size_type>(out.pos));
  return decoded_string;
}

} // namespace

Optional<string> f$zstd_compress(const string &data, int64_t level) noexcept {
  const int min_level = ZSTD_minCLevel();
  const int max_level = ZSTD_maxCLevel();
  if (min_level > level || level > max_level) {
    php_warning("zstd_compress: compression level (%" PRIi64 ") must be within %d..%d or equal to 0", level, min_level, max_level);
    return false;
  }

  return zstd_compress_impl(data, level);
}

Optional<string> f$zstd_uncompress(const string &data) noexcept {
  return zstd_uncompress_impl(data);
}

Optional<string> f$zstd_compress_dict(const string &data, const string &dict) noexcept {
  return zstd_compress_impl(data, DEFAULT_COMPRESS_LEVEL, dict);
}

Optional<string> f$zstd_uncompress_dict(const string &data, const string &dict) noexcept {
  return zstd_uncompress_impl(data, dict);
}
