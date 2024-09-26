// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/utils/concepts.h"

namespace tl {

inline constexpr auto SMALL_STRING_SIZE_LEN = 1;
inline constexpr auto MEDIUM_STRING_SIZE_LEN = 3;
inline constexpr auto LARGE_STRING_SIZE_LEN = 7;

inline constexpr uint64_t SMALL_STRING_MAX_LEN = 253;
inline constexpr uint64_t MEDIUM_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 24) - 1;
[[maybe_unused]] inline constexpr uint64_t LARGE_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 56) - 1;

inline constexpr uint8_t LARGE_STRING_MAGIC = 0xff;
inline constexpr uint8_t MEDIUM_STRING_MAGIC = 0xfe;

class TLBuffer final : private vk::not_copyable {
  string_buffer m_buffer;
  size_t m_pos{0};
  size_t m_remaining{0};

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
    php_assert(pos <= size());
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

  template<standard_layout T, standard_layout U>
  requires std::convertible_to<U, T> void store_trivial(const U &t) noexcept {
    // Here we rely on that endianness of architecture is Little Endian
    store_bytes(reinterpret_cast<const char *>(std::addressof(t)), sizeof(T));
  }

  void store_string(std::string_view s) noexcept;

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

  std::string_view fetch_string() noexcept;
};

template<typename T>
concept tl_serializable = std::default_initializable<T> && requires(T t, TLBuffer tlb) {
  { t.store(tlb) } noexcept -> std::same_as<void>;
};

template<typename T>
concept tl_deserializable = std::default_initializable<T> && requires(T t, TLBuffer tlb) {
  { t.fetch(tlb) } noexcept -> std::convertible_to<bool>;
};

} // namespace tl
