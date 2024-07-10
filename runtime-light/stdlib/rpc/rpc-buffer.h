// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/utils/concepts.h"

class RpcBuffer : private vk::not_copyable {
  string_buffer m_buffer;
  size_t m_pos{0};
  size_t m_remaining{0};

public:
  RpcBuffer() = default;

  const char *data() const noexcept {
    return m_buffer.buffer();
  }

  size_t size() const noexcept {
    return static_cast<size_t>(m_buffer.size());
  }

  size_t remaining() const noexcept {
    return m_remaining;
  }

  size_t pos() const noexcept {
    return m_pos;
  }

  void clean() noexcept {
    m_buffer.clean();
    m_pos = 0;
    m_remaining = 0;
  }

  void reset(size_t pos) noexcept {
    php_assert(pos >= 0 && pos <= size());
    m_pos = pos;
    m_remaining = size() - m_pos;
  }

  void adjust(size_t len) noexcept {
    php_assert(m_pos + len <= size());
    m_pos += len;
    m_remaining -= len;
  }

  void store(const char *src, size_t len) noexcept {
    m_buffer.append(src, len);
    m_remaining += len;
  }

  template<standard_layout T>
  void store_trivial(const T &t) noexcept {
    store(reinterpret_cast<const char *>(&t), sizeof(T));
  }

  template<standard_layout T>
  std::optional<T> fetch_trivial() noexcept {
    if (m_remaining < sizeof(T)) {
      return std::nullopt;
    }

    const auto t{*reinterpret_cast<const T *>(m_buffer.c_str() + m_pos)};
    m_pos += sizeof(T);
    m_remaining -= sizeof(T);
    return t;
  }
};
