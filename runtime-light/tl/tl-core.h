// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/utils/concepts.h"

namespace tl {

class TLBuffer final {
  static constexpr auto INIT_BUFFER_SIZE = 1024;

  kphp::stl::vector<char, kphp::memory::script_allocator> m_buffer;
  size_t m_pos{0};
  size_t m_remaining{0};

public:
  TLBuffer() noexcept {
    m_buffer.reserve(INIT_BUFFER_SIZE);
  }

  TLBuffer(const TLBuffer &) = delete;

  TLBuffer &operator=(const TLBuffer &) = delete;

  TLBuffer(TLBuffer &&oth) noexcept
    : m_buffer(std::exchange(oth.m_buffer, {}))
    , m_pos(std::exchange(oth.m_pos, 0))
    , m_remaining(std::exchange(oth.m_pos, 0)) {}

  TLBuffer &operator=(TLBuffer &&) = delete;

  ~TLBuffer() = default;

  const char *data() const noexcept {
    return m_buffer.data();
  }

  size_t size() const noexcept {
    return m_buffer.size();
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
    php_assert(pos <= size());
    m_pos = pos;
    m_remaining = size() - m_pos;
  }

  void adjust(size_t len) noexcept {
    php_assert(m_pos + len <= size());
    m_pos += len;
    m_remaining -= len;
  }

  void store_bytes(std::string_view bytes_view) noexcept {
    // TODO: use std::vector::append_range after switch to C++-23
    m_buffer.insert(m_buffer.end(), bytes_view.cbegin(), bytes_view.cend());
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
  requires std::convertible_to<U, T> void store_trivial(const U &t) noexcept {
    store_bytes({reinterpret_cast<const char *>(std::addressof(t)), sizeof(T)});
  }

  template<standard_layout T>
  std::optional<T> fetch_trivial() noexcept {
    if (remaining() < sizeof(T)) {
      return std::nullopt;
    }

    auto t{*reinterpret_cast<const T *>(std::next(data(), pos()))};
    adjust(sizeof(T));
    return t;
  }

  template<standard_layout T>
  std::optional<T> lookup_trivial() const noexcept {
    if (remaining() < sizeof(T)) {
      return std::nullopt;
    }
    return *reinterpret_cast<const T *>(std::next(data(), pos()));
  }
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
