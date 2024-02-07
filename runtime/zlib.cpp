// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/zlib.h"

#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

namespace {
voidpf zlib_static_alloc(voidpf opaque, uInt items, uInt size) {
  int *buf_pos = (int *)opaque;
  php_assert (items != 0 && (PHP_BUF_LEN - *buf_pos) / items >= size);

  int pos = *buf_pos;
  *buf_pos += items * size;
  php_assert (*buf_pos <= PHP_BUF_LEN);
  return php_buf + pos;
}

void zlib_static_free(voidpf opaque __attribute__((unused)), voidpf address __attribute__((unused))) {}

voidpf zlib_dynamic_alloc(voidpf opaque __attribute__((unused)), uInt items, uInt size) {
  return static_cast<voidpf>(dl::script_allocator_calloc(items, size));
}

void zlib_dynamic_free(voidpf opaque __attribute__((unused)), voidpf address) {
  dl::script_allocator_free(address);
}
} // namespace

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding) {
  int buf_pos = 0;
  z_stream strm;
  strm.zalloc = zlib_static_alloc;
  strm.zfree = zlib_static_free;
  strm.opaque = &buf_pos;

  unsigned int res_len = (unsigned int)compressBound(s_len) + 30;
  static_SB.clean().reserve(res_len);

  dl::enter_critical_section();//OK
  int ret = deflateInit2 (&strm, level, Z_DEFLATED, encoding, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (ret == Z_OK) {
    strm.avail_in = (unsigned int)s_len;
    strm.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (s));
    strm.avail_out = res_len;
    strm.next_out = reinterpret_cast <Bytef *> (static_SB.buffer());

    ret = deflate(&strm, Z_FINISH);
    deflateEnd(&strm);

    if (ret == Z_STREAM_END) {
      dl::leave_critical_section();

      static_SB.set_pos(static_cast<int64_t>(strm.total_out));
      return &static_SB;
    }
  }
  dl::leave_critical_section();

  php_warning("Error during pack of string with length %d", s_len);

  static_SB.clean();
  return &static_SB;
}

class_instance<C$DeflateContext> f$deflate_init(int64_t encoding, const array<mixed> &options) {
  int level = -1;
  int memory = 8;
  int window = 15;
  int strategy = Z_DEFAULT_STRATEGY;
  auto extract_int_option = [&](int lbound, int ubound, const array_iterator<const mixed> & option, int & dst) {
    mixed value = option.get_value();
    if (value.is_int() && value.as_int() >= lbound && value.as_int() <= ubound) {
      dst = value.as_int();
      return 0;
    } else {
      php_warning("deflate_init() : option %s should be number between %d..%d", option.get_string_key().c_str(), lbound, ubound);
      return -1;
    }
  };
  switch (encoding) {
    case ZLIB_ENCODING_RAW:
    case ZLIB_ENCODING_DEFLATE:
    case ZLIB_ENCODING_GZIP:
      break;
    default:
      php_warning("deflate_init() : encoding should be one of ZLIB_ENCODING_RAW, ZLIB_ENCODING_DEFLATE, ZLIB_ENCODING_GZIP");
      return {};
  }
  for (const auto &option : options) {
    mixed value;
    if (!option.is_string_key()) {
      php_warning("deflate_init() : unsupported option");
      return {};
    }
    if (option.get_string_key() == string("level")) {
      if (extract_int_option(-1, 9, option, level) != 0) {
        return {};
      }
    } else if (option.get_string_key() == string("memory")) {
      if (extract_int_option(1, 9, option, memory) != 0) {
        return {};
      }
    } else if (option.get_string_key() == string("window")) {
      if (extract_int_option(8, 15, option, window) != 0) {
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
            php_warning("deflate_init() : option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
            return {};
        }
      } else {
        php_warning("deflate_init() : option strategy should be one of ZLIB_FILTERED, ZLIB_HUFFMAN_ONLY, ZLIB_RLE, ZLIB_FIXED or ZLIB_DEFAULT_STRATEGY");
        return {};
      }
    } else if (option.get_string_key() == string("dictionary")) {
      php_warning("deflate_init() : option dictionary isn't supported yet");
      return {};
    } else {
      php_warning("deflate_init() : unknown option name \"%s\"", option.get_string_key().c_str());
      return {};
    }
  }

  class_instance<C$DeflateContext> context;
  context.alloc();

  z_stream *stream = &context.get()->stream;
  stream->zalloc = zlib_dynamic_alloc;
  stream->zfree = zlib_dynamic_free;
  stream->opaque = nullptr;

  if (encoding < 0) {
    encoding += 15 - window;
  } else {
    encoding -= 15 - window;
  }

  dl::CriticalSectionGuard guard;
  int err = deflateInit2(stream, level, Z_DEFLATED, encoding, memory, strategy);
  if (err != Z_OK) {
    php_warning("deflate_init() : zlib error %s", zError(err));
    context.destroy();
    return {};
  }
  return context;
}

Optional<string> f$deflate_add(const class_instance<C$DeflateContext> &context, const string &data, int64_t flush_type) {
  switch (flush_type) {
    case Z_BLOCK:
    case Z_NO_FLUSH:
    case Z_PARTIAL_FLUSH:
    case Z_SYNC_FLUSH:
    case Z_FULL_FLUSH:
    case Z_FINISH:
      break;
    default:
      php_warning("deflate_add() : flush type should be one of ZLIB_NO_FLUSH, ZLIB_PARTIAL_FLUSH, ZLIB_SYNC_FLUSH, ZLIB_FULL_FLUSH, ZLIB_FINISH, ZLIB_BLOCK, ZLIB_TREES");
      return {};
  }

  dl::CriticalSectionGuard guard;
  z_stream *stream = &context.get()->stream;
  int out_size = deflateBound(stream, data.size()) + 30;
  out_size = out_size < 64 ? 64 : out_size;
  char * buffer = static_cast<char *>(dl::script_allocator_malloc(out_size));
  auto finalizer = vk::finally([buffer](){dl::script_allocator_free(buffer);});
  stream->next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(data.c_str()));
  stream->next_out = reinterpret_cast<Bytef *>(buffer);
  stream->avail_in = data.size();
  stream->avail_out = out_size;

  int status = Z_OK;
  int buffer_used = 0;
  do {
    if (stream->avail_out == 0) {
      buffer = static_cast<char *>(dl::script_allocator_realloc(buffer, out_size + 64));
      out_size += 64;
      stream->avail_out = 64;
      stream->next_out = reinterpret_cast<Bytef *>(buffer + buffer_used);
    }
    status = deflate(stream, flush_type);
    buffer_used = out_size - stream->avail_out;
  } while (status == Z_OK && stream->avail_out == 0);

  int len;
  switch (status) {
    case Z_OK:
      len = stream->next_out - reinterpret_cast<Bytef *>(buffer);
      return string(buffer, len);
    case Z_STREAM_END:
      len = stream->next_out - reinterpret_cast<Bytef *>(buffer);
      deflateReset(stream);
      return string(buffer, len);
    default:
      php_warning("deflate_add() : zlib error %s", zError(status));
      return {};
  }
}

string f$gzcompress(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %" PRIi64 " in function gzcompress", level);
    level = 6;
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_ENCODING_DEFLATE)->str();
}

string f$gzdeflate(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %" PRIi64 " in function gzcompress", level);
    level = 6;
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_ENCODING_RAW)->str();
}

string f$gzencode(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %" PRIi64 " in function gzencode", level);
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_ENCODING_GZIP)->str();
}

static string::size_type zlib_decode_raw(vk::string_view s, int encoding) {
  dl::enter_critical_section(); // OK

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = s.size();
  strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(s.data()));
  strm.avail_out = PHP_BUF_LEN;
  strm.next_out = reinterpret_cast<Bytef *>(php_buf);

  int ret = inflateInit2(&strm, encoding);
  if (ret != Z_OK) {
    dl::leave_critical_section();
    return -1;
  }

  ret = inflate(&strm, Z_NO_FLUSH);
  switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
    case Z_STREAM_ERROR:
      inflateEnd(&strm);
      dl::leave_critical_section();

      php_assert(ret != Z_STREAM_ERROR);
      return -1;
  }

  int res_len = PHP_BUF_LEN - strm.avail_out;

  if (strm.avail_out == 0 && ret != Z_STREAM_END) {
    inflateEnd(&strm);
    dl::leave_critical_section();

    php_warning("size of unpacked data is greater then %d. Can't decode.", PHP_BUF_LEN);
    return -1;
  }

  inflateEnd(&strm);
  dl::leave_critical_section();
  return res_len;
}

const char *gzuncompress_raw(vk::string_view s, string::size_type *result_len) {
  auto len = zlib_decode_raw(s, ZLIB_ENCODING_DEFLATE);
  if (len == -1u) {
    *result_len = 0;
    return "";
  }
  *result_len = len;
  return php_buf;
}

string zlib_decode(const string &s, int encoding) {
  int len = zlib_decode_raw({s.c_str(), s.size()}, encoding);
  if (len == -1u) {
    return {};
  }
  return {php_buf, static_cast<string::size_type>(len)};
}

string f$gzdecode(const string &s) {
  return zlib_decode(s, ZLIB_ENCODING_GZIP);
}

string f$gzuncompress(const string &s) {
  return zlib_decode(s, ZLIB_ENCODING_DEFLATE);
}

string f$gzinflate(const string &s) {
  return zlib_decode(s, ZLIB_ENCODING_RAW);
}
