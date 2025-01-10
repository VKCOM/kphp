// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <zstd.h>

#include "runtime-common/core/runtime-core.h"
#include "runtime/dummy-visitor-methods.h"

constexpr int DEFAULT_COMPRESS_LEVEL = 3;

Optional<string> f$zstd_compress(const string &data, int64_t level = DEFAULT_COMPRESS_LEVEL) noexcept;

Optional<string> f$zstd_uncompress(const string &data) noexcept;

Optional<string> f$zstd_compress_dict(const string &data, const string &dict) noexcept;

Optional<string> f$zstd_uncompress_dict(const string &data, const string &dict) noexcept;

struct C$ZstdCtx : public refcountable_php_classes<C$ZstdCtx>, private DummyVisitorMethods {
  C$ZstdCtx() = default;
  using DummyVisitorMethods::accept;

  ~C$ZstdCtx() {
    dl::CriticalSectionGuard guard;
    // TODO
  }

  ZSTD_CStream *ctx;
};

class_instance<C$ZstdCtx> f$zstd_stream_init(const array<mixed> &options = {}) noexcept;

Optional<string> f$zstd_stream_add(const class_instance<C$ZstdCtx> &ctx, const string &data, bool end) noexcept;
