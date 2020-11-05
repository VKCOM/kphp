// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/zlib.h"

#include <zlib.h>

#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

static voidpf zlib_alloc(voidpf opaque, uInt items, uInt size) {
  int *buf_pos = (int *)opaque;
  php_assert (items != 0 && (PHP_BUF_LEN - *buf_pos) / items >= size);

  int pos = *buf_pos;
  *buf_pos += items * size;
  php_assert (*buf_pos <= PHP_BUF_LEN);
  return php_buf + pos;
}

static void zlib_free(voidpf opaque __attribute__((unused)), voidpf address __attribute__((unused))) {
}

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding) {
  int buf_pos = 0;
  z_stream strm;
  strm.zalloc = zlib_alloc;
  strm.zfree = zlib_free;
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

string f$gzcompress(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %ld in function gzcompress", level);
    level = 6;
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_COMPRESS)->str();
}

string f$gzdeflate(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %ld in function gzcompress", level);
    level = 6;
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_RAW)->str();
}

string f$gzencode(const string &s, int64_t level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %ld in function gzencode", level);
  }

  return zlib_encode(s.c_str(), s.size(), static_cast<int32_t>(level), ZLIB_ENCODE)->str();
}

static string::size_type zlib_decode_raw(vk::string_view s, int encoding) {
  dl::enter_critical_section();//OK

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = s.size();
  strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(s.data()));
  strm.avail_out = PHP_BUF_LEN;
  strm.next_out = reinterpret_cast <Bytef *> (php_buf);

  int ret = inflateInit2 (&strm, encoding);
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

      php_assert (ret != Z_STREAM_ERROR);
      return -1;
  }

  int res_len = PHP_BUF_LEN - strm.avail_out;

  if (strm.avail_out == 0 && ret != Z_STREAM_END) {
    inflateEnd(&strm);
    dl::leave_critical_section();

    php_critical_error ("size of unpacked data is greater then %d. Can't decode.", PHP_BUF_LEN);
    return -1;
  }

  inflateEnd(&strm);
  dl::leave_critical_section();
  return res_len;
}

const char *gzuncompress_raw(vk::string_view s, string::size_type *result_len) {
  auto len = zlib_decode_raw(s, ZLIB_COMPRESS);
  if (len == -1u) {
    *result_len = 0;
    return "";
  }
  *result_len = len;
  return php_buf;
}


string zlib_decode(const string&s, int encoding) {
  int len = zlib_decode_raw({s.c_str(), s.size()}, encoding);
  if (len == -1u) {
    return string();
  }
  return string(php_buf, len);
}

string f$gzdecode(const string &s) {
  return zlib_decode(s, ZLIB_ENCODE);
}

string f$gzuncompress(const string &s) {
  return zlib_decode(s, ZLIB_COMPRESS);
}

string f$gzinflate(const string &s) {
  return zlib_decode(s, ZLIB_RAW);
}
