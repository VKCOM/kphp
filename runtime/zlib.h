// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <zlib.h>

#include "common/wrappers/string_view.h"

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"
#include "runtime/dummy-visitor-methods.h"
#include "runtime/streams.h"

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

struct C$InflateContext : public refcountable_php_classes<C$DeflateContext>, private DummyVisitorMethods {
  C$InflateContext() = default;
  using DummyVisitorMethods::accept;

  ~C$InflateContext() {
    dl::CriticalSectionGuard guard;
    inflateEnd(&stream);
  }

  z_stream stream{};
};

const string_buffer *zlib_encode(const char *s, int32_t s_len, int32_t level, int32_t encoding);//returns pointer to static_SB

const char *gzuncompress_raw(vk::string_view s, string::size_type *result_len);

/*
 * initialize context for incremental compression
 */
class_instance<C$DeflateContext> f$deflate_init(int64_t encoding, const array<mixed> &options = {});

/*
 * incremental data compression
 */
Optional<string> f$deflate_add(const class_instance<C$DeflateContext> &context, const Optional<string> &data, int64_t flush_type = Z_SYNC_FLUSH);

/*
 * initialize context for incremental uncompression
 */
class_instance<C$InflateContext> f$inflate_init(int64_t encoding, const array<mixed> &options = {});

/*
 * incremental data uncompression
 */
Optional<string> f$inflate_add(const class_instance<C$InflateContext> &context, const Optional<string> &data, int64_t flush_type = Z_SYNC_FLUSH);

/*
 * compress a string
 */
string f$gzcompress(const string &s, int64_t level = -1);

/*
 * uncompress a compressed string
 */
string f$gzuncompress(const string &s);

/*
 * create a gzip compressed string
 */
string f$gzencode(const string &s, int64_t level = -1);

/*
 * decode a gzip compressed string
 */
string f$gzdecode(const string &s);

/*
 * deflate a string
 */
string f$gzdeflate(const string &s, int64_t level = -1);

/*
 * inflate a deflated string
 */
string f$gzinflate(const string &s);

/*
 * output a gz-file
 */
int64_t f$readgzfile(const string &s); // parameter $use_include_path dropped

/*
 * open a gz-file for reading or writing
 */
Stream f$gzopen(const string &s, const string &mode); // parameter $use_include_path dropped

/*
 * write the contents of $text to the given gz-file
 */
Optional<int64_t> f$gzwrite(const Stream &stream, const string &text); // parameter $length dropped

/*
 * set the file position indicator for the given file pointer to the given offset byte
 */
int64_t f$gzseek(const Stream &stream, int64_t offset, int64_t whence = 0);

/*
 * set the file position indicator of the given gz-file pointer to the beginning of the file stream
 */
bool f$gzrewind(const Stream &stream);

/*
 * get the position of the given file pointer
 */
Optional<int64_t> f$gztell(const Stream &stream);

/*
 * read up to $length bytes from the given gz-file pointer
 */
Optional<string> f$gzread(const Stream &stream, int64_t length);

/*
 * returns a string containing a single character read from the given gz-file pointer
 */
Optional<string> f$gzgetc(const Stream &stream);

/*
 * get a string of up to ($length - 1) bytes read from the given file pointer
 */
Optional<string> f$gzgets(const Stream &stream, int64_t length = -1);

/*
 * print from the current position of the given gz-file pointer until EOF
 */
Optional<int64_t> f$gzpassthru(const Stream &stream);

/*
 * test the given gz-file pointer for EOF
 */
bool f$gzeof(const Stream &stream);

/*
 * close the given gz-file pointer
 */
bool f$gzclose(const Stream &stream);

