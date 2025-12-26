// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "zlib/zlib.h"

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$DeflateContext : public refcountable_php_classes<C$DeflateContext>, private DummyVisitorMethods {
  C$DeflateContext() noexcept = default;
  using DummyVisitorMethods::accept;

  C$DeflateContext(const C$DeflateContext&) = delete;
  C$DeflateContext(C$DeflateContext&&) = delete;
  C$DeflateContext& operator=(const C$DeflateContext&) = delete;
  C$DeflateContext& operator=(C$DeflateContext&&) = delete;

  ~C$DeflateContext() {
    deflateEnd(std::addressof(stream));
  }

  z_stream stream{};
};
