// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "zlib/zlib.h"

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

class C$DeflateContext : public refcountable_php_classes<C$DeflateContext>, private DummyVisitorMethods {
  std::optional<z_stream> m_stream;

  static voidpf dynamic_calloc([[maybe_unused]] voidpf opaque, uInt items, uInt size) noexcept {
    auto* mem{kphp::memory::script::calloc(items, size)};
    if (mem == nullptr) [[unlikely]] {
      kphp::log::warning("zlib dynamic calloc: can't allocate {} bytes", items * size);
    }
    return mem;
  }

  static void dynamic_free([[maybe_unused]] voidpf opaque, voidpf address) noexcept {
    kphp::memory::script::free(address);
  }

public:
  using DummyVisitorMethods::accept;
  C$DeflateContext() noexcept = default;

  C$DeflateContext(const C$DeflateContext&) = delete;
  C$DeflateContext(C$DeflateContext&&) = delete;
  C$DeflateContext& operator=(const C$DeflateContext&) = delete;
  C$DeflateContext& operator=(C$DeflateContext&&) = delete;

  bool init(int32_t level, int32_t window_bits, int32_t memory, int32_t strategy) noexcept {
    z_stream* stream{std::addressof(m_stream.emplace(z_stream{.zalloc = dynamic_calloc, .zfree = dynamic_free, .opaque = nullptr}))};

    if (auto err{deflateInit2(stream, level, Z_DEFLATED, window_bits, memory, strategy)}; err != Z_OK) {
      kphp::log::warning("zlib error {}", err);
      return false;
    }

    return true;
  }

  z_stream& operator*() noexcept {
    kphp::log::assertion(m_stream.has_value());
    return *m_stream;
  }

  ~C$DeflateContext() {
    if (m_stream.has_value()) [[likely]] {
      deflateEnd(std::addressof(*m_stream));
    }
  }
};
