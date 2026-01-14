// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <optional>

#include "zlib/zlib.h"

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$DeflateContext : public refcountable_php_classes<C$DeflateContext>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;
  C$DeflateContext() noexcept = default;

  C$DeflateContext(const C$DeflateContext&) = delete;
  C$DeflateContext(C$DeflateContext&&) = delete;
  C$DeflateContext& operator=(const C$DeflateContext&) = delete;
  C$DeflateContext& operator=(C$DeflateContext&&) = delete;

  ~C$DeflateContext() {
    if (stream.has_value()) [[likely]] {
      deflateEnd(std::addressof(*stream));
    }
  }

  std::optional<z_stream> stream;
};
