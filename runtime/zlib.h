#pragma once

#include "runtime/kphp_core.h"

constexpr int32_t ZLIB_COMPRESS = 0x0f;
constexpr int32_t ZLIB_ENCODE = 0x1f;

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding);//returns pointer to static_SB

string f$gzcompress(const string &s, int64_t level = -1);

const char *gzuncompress_raw(const char *s, int s_len, string::size_type *result_len);

string f$gzuncompress(const string &s);

string f$gzencode(const string &s, int64_t level = -1);

string f$gzdecode(const string &s);
