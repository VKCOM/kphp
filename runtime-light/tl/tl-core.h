// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include "common/algorithms/find.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"
#include "runtime-light/utils/concepts.h"

namespace tl {

inline bool is_int32_overflow(int64_t v) noexcept {
  const auto v32{static_cast<int32_t>(v)};
  return vk::none_of_equal(v, int64_t{v32}, int64_t{static_cast<uint32_t>(v32)});
}

class storer {
  static constexpr auto DEFAULT_BUFFER_CAPACITY = 1024;

  kphp::stl::vector<std::byte, kphp::memory::script_allocator> m_buffer;

public:
  explicit storer(size_t capacity = DEFAULT_BUFFER_CAPACITY) noexcept {
    m_buffer.reserve(capacity);
  }

  storer(storer&& other) noexcept
      : m_buffer(std::move(other.m_buffer)) {}

  storer& operator=(storer&& other) noexcept {
    if (this != std::addressof(other)) {
      m_buffer = std::move(other.m_buffer);
    }
    return *this;
  }

  ~storer() = default;

  storer(const storer&) = delete;
  storer& operator=(const storer&) = delete;

  std::span<const std::byte> view() const noexcept {
    return m_buffer;
  }

  void clear() noexcept {
    m_buffer.clear();
  }

  void reserve(size_t capacity) noexcept {
    m_buffer.reserve(capacity);
  }

  void store_bytes(std::span<const std::byte> bytes) noexcept {
    m_buffer.append_range(bytes);
  }

  template<standard_layout T, standard_layout U>
  requires std::convertible_to<U, T>
  void store_trivial(const U& t) noexcept {
    store_bytes({reinterpret_cast<const std::byte*>(std::addressof(t)), sizeof(T)});
  }
};

class fetcher {
  size_t m_pos{};
  size_t m_remaining;
  std::span<const std::byte> m_buffer;

public:
  explicit fetcher(std::span<const std::byte> buffer) noexcept
      : m_remaining(buffer.size()),
        m_buffer(buffer) {}

  fetcher(const fetcher& other) noexcept = default;

  fetcher(fetcher&& other) noexcept
      : m_pos(std::exchange(other.m_pos, 0)),
        m_remaining(std::exchange(other.m_remaining, 0)),
        m_buffer(std::exchange(other.m_buffer, {})) {}

  fetcher& operator=(const fetcher& other) noexcept = default;

  fetcher& operator=(fetcher&& other) noexcept {
    if (this != std::addressof(other)) {
      m_pos = std::exchange(other.m_pos, 0);
      m_remaining = std::exchange(other.m_remaining, 0);
      m_buffer = std::exchange(other.m_buffer, {});
    }
    return *this;
  }

  ~fetcher() = default;

  std::span<const std::byte> view() const noexcept {
    return m_buffer;
  }

  size_t pos() const noexcept {
    return m_pos;
  }

  void reset(size_t pos) noexcept {
    kphp::log::assertion(pos <= m_buffer.size());
    m_pos = pos;
    m_remaining = m_buffer.size() - m_pos;
  }

  size_t remaining() const noexcept {
    return m_remaining;
  }

  void adjust(size_t len) noexcept {
    kphp::log::assertion(m_pos + len <= m_buffer.size());
    m_pos += len;
    m_remaining -= len;
  }

  std::optional<std::span<const std::byte>> fetch_bytes(size_t len) noexcept {
    if (m_remaining < len) {
      return std::nullopt;
    }
    auto bytes{m_buffer.subspan(m_pos, len)};
    adjust(len);
    return bytes;
  }

  template<standard_layout T>
  std::optional<T> fetch_trivial() noexcept {
    if (m_remaining < sizeof(T)) {
      return std::nullopt;
    }
    auto t{*reinterpret_cast<const T*>(std::next(m_buffer.data(), m_pos))};
    adjust(sizeof(T));
    return t;
  }
};

template<typename T>
concept serializable = std::default_initializable<T> && requires(T t, tl::storer tls) {
  { t.store(tls) } noexcept -> std::same_as<void>;
};

template<typename T>
concept deserializable = std::default_initializable<T> && requires(T t, tl::fetcher tlf) {
  { t.fetch(tlf) } noexcept -> std::convertible_to<bool>;
};

template<typename T>
concept footprintable = std::default_initializable<T> && requires(T t) {
  { t.footprint() } noexcept -> std::same_as<size_t>;
};

} // namespace tl
