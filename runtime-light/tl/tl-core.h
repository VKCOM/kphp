// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "common/algorithms/find.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/utils/concepts.h"

namespace tl {

inline bool is_int32_overflow(int64_t v) noexcept {
  const auto v32{static_cast<int32_t>(v)};
  return vk::none_of_equal(v, int64_t{v32}, int64_t{static_cast<uint32_t>(v32)});
}

class TLBuffer final {
  static constexpr auto INIT_BUFFER_SIZE = 1024;

  size_t m_pos{0};
  size_t m_remaining{0};
  kphp::stl::vector<char, kphp::memory::script_allocator> m_buffer;

public:
  explicit TLBuffer(size_t capacity = INIT_BUFFER_SIZE) noexcept {
    m_buffer.reserve(capacity);
  }

  TLBuffer(const TLBuffer&) = delete;

  TLBuffer& operator=(const TLBuffer&) = delete;

  TLBuffer(TLBuffer&& oth) noexcept
      : m_pos(std::exchange(oth.m_pos, 0)),
        m_remaining(std::exchange(oth.m_pos, 0)),
        m_buffer(std::exchange(oth.m_buffer, {})) {}

  TLBuffer& operator=(TLBuffer&& oth) noexcept {
    if (this != std::addressof(oth)) [[likely]] {
      m_pos = std::exchange(oth.m_pos, 0);
      m_remaining = std::exchange(oth.m_remaining, 0);
      m_buffer = std::exchange(oth.m_buffer, {});
    }
    return *this;
  }

  ~TLBuffer() = default;

  const char* data() const noexcept {
    return m_buffer.data();
  }

  size_t size() const noexcept {
    return m_buffer.size();
  }

  size_t capacity() const noexcept {
    return m_buffer.capacity();
  }

  bool empty() const noexcept {
    return m_buffer.empty();
  }

  size_t pos() const noexcept {
    return m_pos;
  }

  size_t remaining() const noexcept {
    return m_remaining;
  }

  void clean() noexcept {
    m_buffer.clear();
    m_pos = 0;
    m_remaining = 0;
  }

  void reset(size_t pos) noexcept {
    kphp::log::assertion(pos <= size());
    m_pos = pos;
    m_remaining = size() - m_pos;
  }

  void adjust(size_t len) noexcept {
    kphp::log::assertion(m_pos + len <= size());
    m_pos += len;
    m_remaining -= len;
  }

  void store_bytes(std::string_view bytes_view) noexcept {
    m_buffer.append_range(bytes_view);
    m_remaining += bytes_view.size();
  }

  std::optional<std::string_view> fetch_bytes(size_t len) noexcept {
    if (len > remaining()) {
      return {};
    }
    std::string_view bytes_view{std::next(data(), pos()), len};
    adjust(len);
    return bytes_view;
  }

  template<standard_layout T, standard_layout U>
  requires std::convertible_to<U, T>
  void store_trivial(const U& t) noexcept {
    store_bytes({reinterpret_cast<const char*>(std::addressof(t)), sizeof(T)});
  }

  template<standard_layout T>
  std::optional<T> fetch_trivial() noexcept {
    if (remaining() < sizeof(T)) {
      return std::nullopt;
    }

    auto t{*reinterpret_cast<const T*>(std::next(data(), pos()))};
    adjust(sizeof(T));
    return t;
  }

  template<standard_layout T>
  std::optional<T> lookup_trivial() const noexcept {
    if (remaining() < sizeof(T)) {
      return std::nullopt;
    }
    return *reinterpret_cast<const T*>(std::next(data(), pos()));
  }
};

template<typename T>
concept serializable = std::default_initializable<T> && requires(T t, TLBuffer tlb) {
  { t.store(tlb) } noexcept -> std::same_as<void>;
};

template<typename T>
concept deserializable = std::default_initializable<T> && requires(T t, TLBuffer tlb) {
  { t.fetch(tlb) } noexcept -> std::convertible_to<bool>;
};

template<typename T>
concept footprintable = std::default_initializable<T> && requires(T t) {
  { t.footprint() } noexcept -> std::same_as<size_t>;
};

} // namespace tl
