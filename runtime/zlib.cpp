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

const string_buffer *zlib_encode(const char *s, int s_len, int level, int encoding) {
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

      static_SB.set_pos((int)strm.total_out);
      return &static_SB;
    }
  }
  dl::leave_critical_section();

  php_warning("Error during pack of string with length %d", s_len);

  static_SB.clean();
  return &static_SB;
}

string f$gzcompress(const string &s, int level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %d in function gzcompress", level);
    level = 6;
  }

  return zlib_encode(s.c_str(), s.size(), level, ZLIB_COMPRESS)->str();
}

const char *gzuncompress_raw(const char *s, int s_len, string::size_type *result_len) {
  unsigned long res_len = PHP_BUF_LEN;

  dl::enter_critical_section();//OK
  int uncompress_res = uncompress(reinterpret_cast <unsigned char *> (php_buf), &res_len, reinterpret_cast <const unsigned char *> (s), (unsigned long)s_len);
  if (uncompress_res == Z_OK) {
    dl::leave_critical_section();
    *result_len = res_len;
    return php_buf;
  }
  dl::leave_critical_section();

  php_warning("Error during unpack of string of length %d, msg: '%s'", (int)s_len, zError(uncompress_res));
  *result_len = 0;
  return "";
}


string f$gzuncompress(const string &s) {
  string::size_type res_len;
  const char *res = gzuncompress_raw(s.c_str(), s.size(), &res_len);
  return string(res, res_len);
}

string f$gzencode(const string &s, int level) {
  if (level < -1 || level > 9) {
    php_warning("Wrong parameter level = %d in function gzencode", level);
  }

  return zlib_encode(s.c_str(), s.size(), level, ZLIB_ENCODE)->str();
}

string f$gzdecode(const string &s) {
  dl::enter_critical_section();//OK

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = s.size();
  strm.next_in = reinterpret_cast <Bytef *> (const_cast <char *> (s.c_str()));
  strm.avail_out = PHP_BUF_LEN;
  strm.next_out = reinterpret_cast <Bytef *> (php_buf);

  int ret = inflateInit2 (&strm, 15 + 32);
  if (ret != Z_OK) {
    dl::leave_critical_section();
    return string();
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
      return string();
  }

  int res_len = PHP_BUF_LEN - strm.avail_out;

  if (strm.avail_out == 0 && ret != Z_STREAM_END) {
    inflateEnd(&strm);
    dl::leave_critical_section();

    php_critical_error ("size of unpacked data is greater then %d. Can't decode.", PHP_BUF_LEN);
    return string();
  }

  inflateEnd(&strm);
  dl::leave_critical_section();
  return string(php_buf, res_len);
}

