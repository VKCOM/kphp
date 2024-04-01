// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl/methods/rwm.h"

#include <memory>
#include <zstd.h>

#include "common/algorithms/arithmetic.h"
#include "common/tl/methods/compression.h"
#include "common/tl/parse.h"

static inline int rwm_do_xor_compress(void *extra __attribute__((unused)), void *data_, int len) {
  auto *data = static_cast<uint8_t *>(data_);
  for (int i = 0; i < len; i++) {
    data[i] ^= 0xbe;
  }
  return 0;
}

struct zstd_compression_extra_t {
  raw_message_t *raw_out;
  ZSTD_CStream *zstd_stream;
  char *buffer;
  size_t buf_len;
};

static inline int tl_raw_msg_zstd_compress(void *_extra, const void *data, int len) {
  auto *extra = static_cast<zstd_compression_extra_t *>(_extra);
  ZSTD_outBuffer output = {.dst = extra->buffer, .size = extra->buf_len, .pos = 0};
  ZSTD_inBuffer input = {.src = data, .size = (size_t)len, .pos = 0};
  while (input.pos < input.size) {
    ZSTD_compressStream(extra->zstd_stream, &output, &input);
    rwm_push_data(extra->raw_out, output.dst, (int)output.pos);
    output.pos = 0;
  }
  return 0;
}

static void raw_zstd_compress(raw_message_t &rwm) {
  raw_message_t out;
  rwm_init(&out, 0);
  static ZSTD_CStream *zstd_stream = nullptr;
  if (zstd_stream == nullptr) {
    zstd_stream = ZSTD_createCStream();
  }
  static char *out_buf = nullptr;
  static size_t buf_len = 0;
  if (out_buf == nullptr) {
    buf_len = ZSTD_CStreamOutSize();
    out_buf = static_cast<char *>(malloc(buf_len));
  }
  ZSTD_initCStream(zstd_stream, 1); // TODO: maybe change compression level?
  zstd_compression_extra_t extra = {.raw_out = &out, .zstd_stream = zstd_stream, .buffer = out_buf, .buf_len = buf_len};
  rwm_process(&rwm, rwm.total_bytes, tl_raw_msg_zstd_compress, &extra);
  ZSTD_outBuffer output = {.dst = extra.buffer, .size = extra.buf_len, .pos = 0};
  while (true) {
    size_t r = ZSTD_endStream(extra.zstd_stream, &output);
    rwm_push_data(extra.raw_out, output.dst, (int)output.pos);
    output.pos = 0;
    if (r == 0) {
      break;
    }
  }

  int len = out.total_bytes;
  rwm_push_data_front(&out, &len, sizeof(int));
  const int zero = 0;
  int pad_len = align4(len) - len;
  rwm_push_data(&out, &zero, pad_len);

  rwm_clear(&rwm);
  rwm_steal(&rwm, &out);
}

struct zstd_decompression_extra_t {
  raw_message_t *raw_out;
  ZSTD_DStream *zstd_stream;
  char *buffer;
  size_t buf_len;
};

static inline int tl_raw_msg_zstd_decompress(void *_extra, const void *data, int len) {
  auto *extra = static_cast<zstd_decompression_extra_t *>(_extra);
  ZSTD_outBuffer output = {.dst = extra->buffer, .size = extra->buf_len, .pos = 0};
  ZSTD_inBuffer input = {.src = data, .size = (size_t)len, .pos = 0};
  while (input.pos < input.size) {
    while (true) {
      ZSTD_decompressStream(extra->zstd_stream, &output, &input);
      rwm_push_data(extra->raw_out, output.dst, (int)output.pos);
      bool end = output.pos < output.size;
      output.pos = 0;
      if (end) {
        break;
      }
    }
  }
  return 0;
}

static void rwm_zstd_decompress(raw_message_t &in) {
  raw_message_t out;
  rwm_init(&out, 0);
  static ZSTD_DStream *zstd_stream = nullptr;
  if (zstd_stream == nullptr) {
    zstd_stream = ZSTD_createDStream();
  }
  static char *out_buf = nullptr;
  static size_t buf_len = 0;
  if (out_buf == nullptr) {
    buf_len = ZSTD_DStreamOutSize();
    out_buf = static_cast<char *>(malloc(buf_len));
  }
  ZSTD_initDStream(zstd_stream);
  zstd_decompression_extra_t extra = {.raw_out = &out, .zstd_stream = zstd_stream, .buffer = out_buf, .buf_len = buf_len};
  int len = 0;
  rwm_fetch_data(&in, &len, sizeof(int));
  rwm_process(&in, len, tl_raw_msg_zstd_decompress, &extra);

  rwm_free(&in);
  rwm_steal(&in, &out);
}

void compress_rwm(raw_message_t &rwm, int version) noexcept {
  assert(version <= COMPRESSION_VERSION_MAX);
  switch (version) {
    case COMPRESSION_VERSION_TEST_XOR: {
      rwm_fork_deep(&rwm);
      rwm_transform_from_offset(&rwm, rwm.total_bytes, 0, rwm_do_xor_compress, nullptr);
      break;
    }
    case COMPRESSION_VERSION_ZSTD: {
      raw_zstd_compress(rwm);
      break;
    }
    default:
      assert(0);
  }
}

void decompress_rwm(raw_message_t &in, int version) noexcept {
  assert(version <= COMPRESSION_VERSION_MAX);
  assert(in.magic == RM_INIT_MAGIC);
  switch (version) {
    case COMPRESSION_VERSION_TEST_XOR: {
      rwm_fork_deep(&in);
      rwm_transform_from_offset(&in, in.total_bytes, 0, rwm_do_xor_compress, nullptr);
      break;
    }
    case COMPRESSION_VERSION_ZSTD: {
      rwm_zstd_decompress(in);
      break;
    }
    default:
      assert(0);
  }
}

void tl_fetch_init_raw_message(raw_message_t *msg) {
  const auto total_bytes = msg->total_bytes;
  return tl_fetch_init(std::make_unique<tl_in_methods_raw_msg>(msg), total_bytes);
}
