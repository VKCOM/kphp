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

namespace tl_core {
constexpr auto SMALL_STRING_SIZE_LEN = 1;
constexpr auto MEDIUM_STRING_SIZE_LEN = 3;
constexpr auto LARGE_STRING_SIZE_LEN = 7;

constexpr uint64_t SMALL_STRING_MAX_LEN = 253;
constexpr uint64_t MEDIUM_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 24) - 1;
[[maybe_unused]] constexpr uint64_t LARGE_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 56) - 1;

constexpr uint8_t LARGE_STRING_MAGIC = 0xff;
constexpr uint8_t MEDIUM_STRING_MAGIC = 0xfe;

class TLBuffer : private vk::not_copyable {
  string_buffer m_buffer;
  size_t m_pos{0};
  size_t m_remaining{0};
  template<standard_layout T>
  void store_trivial(const T &t) noexcept {
    // Here we rely on that endianness of architecture is Little Endian
    store_bytes(reinterpret_cast<const char *>(&t), sizeof(T));
  }

  template<standard_layout T>
  std::optional<T> fetch_trivial() noexcept {
    if (m_remaining < sizeof(T)) {
      return std::nullopt;
    }

    // Here we rely on that endianness of architecture is Little Endian
    const auto t{*reinterpret_cast<const T *>(m_buffer.c_str() + m_pos)};
    m_pos += sizeof(T);
    m_remaining -= sizeof(T);
    return t;
  }

public:
  TLBuffer() = default;

  const char *data() const noexcept {
    return m_buffer.buffer();
  }

  size_t size() const noexcept {
    return static_cast<size_t>(m_buffer.size());
  }

  size_t pos() const noexcept {
    return m_pos;
  }

  size_t remaining() const noexcept {
    return m_remaining;
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

  void store_bytes(const char *src, size_t len) noexcept {
    m_buffer.append(src, len);
    m_remaining += len;
  }

  void store_i32(int32_t v) noexcept {
    store_trivial(v);
  }

  void store_u32(uint32_t v) noexcept {
    store_trivial(v);
  }

  void store_f32(float v) noexcept {
    store_trivial(v);
  }

  void store_i64(int64_t v) noexcept {
    store_trivial(v);
  }

  void store_f64(double v) noexcept {
    store_trivial(v);
  }

  void store_string(const char *str_buf, size_t str_len) noexcept;

  std::optional<int32_t> fetch_i32() noexcept {
    return fetch_trivial<int32_t>();
  }

  std::optional<uint32_t> fetch_u32() noexcept {
    return fetch_trivial<uint32_t>();
  }

  std::optional<float> fetch_f32() noexcept {
    return fetch_trivial<float>();
  }

  std::optional<int64_t> fetch_i64() noexcept {
    return fetch_trivial<int64_t>();
  }

  std::optional<double> fetch_f64() noexcept {
    return fetch_trivial<double>();
  }

  std::pair<const char *, size_t> fetch_string() noexcept;
};

} // namespace tl_core
