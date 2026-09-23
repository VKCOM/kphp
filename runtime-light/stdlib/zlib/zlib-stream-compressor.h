// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "zlib/zlib.h"

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

namespace kphp::zlib {

class stream_compressor final : private vk::not_copyable {
  std::optional<z_stream> m_stream;

  static voidpf dynamic_calloc([[maybe_unused]] voidpf opaque, uInt items, uInt size) noexcept;
  static void dynamic_free([[maybe_unused]] voidpf opaque, voidpf address) noexcept;

public:
  stream_compressor() noexcept = default;
  ~stream_compressor();

  bool init_if_needed(int32_t level, int32_t encoding, int32_t memory, int32_t strategy) noexcept;
  std::optional<string> compress(std::span<const char> data, bool finish) noexcept;
  void close() noexcept;
};

} // namespace kphp::zlib
