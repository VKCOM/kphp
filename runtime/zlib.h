#pragma once

#include "common/wrappers/string_view.h"

#include "runtime/kphp_core.h"

constexpr int32_t ZLIB_RAW = -0x0f;
constexpr int32_t ZLIB_COMPRESS = 0x0f;
constexpr int32_t ZLIB_ENCODE = 0x1f;

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding);//returns pointer to static_SB

string f$gzcompress(const string &s, int64_t level = -1);

const char *gzuncompress_raw(vk::string_view s, string::size_type *result_len);

string f$gzuncompress(const string &s);

string f$gzencode(const string &s, int64_t level = -1);

string f$gzdecode(const string &s);

string f$gzdeflate(const string &s, int64_t level);

string f$gzinflate(const string &s);
