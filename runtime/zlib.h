// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <zlib.h>

#include "common/wrappers/string_view.h"

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"
#include "runtime/dummy-visitor-methods.h"

constexpr int64_t ZLIB_ENCODING_RAW = -0x0f;
constexpr int64_t ZLIB_ENCODING_DEFLATE = 0x0f;
constexpr int64_t ZLIB_ENCODING_GZIP = 0x1f;

struct C$DeflateContext : public refcountable_php_classes<C$DeflateContext>, private DummyVisitorMethods {
  C$DeflateContext() = default;
  using DummyVisitorMethods::accept;

  ~C$DeflateContext() {
    dl::CriticalSectionGuard guard;
    deflateEnd(&stream);
  }

  z_stream stream{};
};

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding);//returns pointer to static_SB

class_instance<C$DeflateContext> f$deflate_init(int64_t encoding, const array<mixed> & options = {});

Optional<string> f$deflate_add(const class_instance<C$DeflateContext> & context, const string & data, int64_t flush_type = Z_SYNC_FLUSH);

string f$gzcompress(const string &s, int64_t level = -1);

const char *gzuncompress_raw(vk::string_view s, string::size_type *result_len);

string f$gzuncompress(const string &s);

string f$gzencode(const string &s, int64_t level = -1);

string f$gzdecode(const string &s);

string f$gzdeflate(const string &s, int64_t level);

string f$gzinflate(const string &s);
