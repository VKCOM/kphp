// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/logs.h"

namespace kphp::component {

class stream {
  using storage_type = std::unique_ptr<std::byte, decltype(std::addressof(kphp::memory::script::free))>;

  storage_type m_storage;
  size_t m_storage_size{};
  size_t m_storage_capacity{};
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};

  stream(storage_type storage, size_t capacity, k2::descriptor descriptor) noexcept
      : m_storage(std::move(storage)),
        m_storage_capacity(capacity),
        m_descriptor(descriptor) {
    if (m_storage_capacity != 0) {
      kphp::log::assertion(static_cast<bool>(m_storage));
    }
  }

public:
  static constexpr auto DEFAULT_STORAGE_CAPACITY = static_cast<size_t>(1 << 11);

  stream(stream&& other) noexcept
      : m_storage(std::move(other.m_storage)),
        m_storage_size(std::exchange(other.m_storage_size, 0)),
        m_storage_capacity(std::exchange(other.m_storage_capacity, 0)),
        m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)) {}

  stream& operator=(stream&& other) noexcept {
    if (this != std::addressof(other)) {
      reset(k2::INVALID_PLATFORM_DESCRIPTOR);
      m_storage = std::move(other.m_storage);
      m_storage_size = std::exchange(other.m_storage_size, 0);
      m_storage_capacity = std::exchange(other.m_storage_capacity, 0);
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
    }
    return *this;
  }

  stream(const stream&) = delete;
  stream& operator=(const stream&) = delete;

  ~stream() {
    reset(k2::INVALID_PLATFORM_DESCRIPTOR);
  }

  static auto open(std::string_view component_name, k2::stream_kind stream_kind, size_t capacity = DEFAULT_STORAGE_CAPACITY) noexcept
      -> std::expected<kphp::component::stream, int32_t>;
  static auto accept(size_t capacity = DEFAULT_STORAGE_CAPACITY, std::chrono::nanoseconds timeout = std::chrono::nanoseconds{0}) noexcept
      -> kphp::coro::task<std::optional<kphp::component::stream>>;

  auto clear() noexcept -> void;
  auto reset(k2::descriptor descriptor) noexcept -> void;
  auto reserve(size_t capacity) noexcept -> std::expected<void, int32_t>;

  auto size() const noexcept -> size_t;
  auto capacity() const noexcept -> size_t;
  auto descriptor() const noexcept -> k2::descriptor;
  auto data() const noexcept -> std::span<const std::byte>;

  auto read() noexcept -> kphp::coro::task<std::expected<void, int32_t>>;
  auto write(std::span<const std::byte> data) const noexcept -> kphp::coro::task<std::expected<void, int32_t>>;
  auto shutdown_write() const noexcept -> void;
};

// ================================================================================================

inline auto stream::open(std::string_view name, k2::stream_kind stream_kind, size_t capacity) noexcept -> std::expected<kphp::component::stream, int32_t> {
  int32_t errc{};
  k2::descriptor descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  switch (stream_kind) {
  case k2::stream_kind::component:
    errc = k2::open(std::addressof(descriptor), name.size(), name.data());
    break;
  case k2::stream_kind::tcp:
    errc = k2::tcp_connect(std::addressof(descriptor), name.data(), name.size());
    break;
  case k2::stream_kind::udp:
    errc = k2::udp_connect(std::addressof(descriptor), name.data(), name.size());
    break;
  }

  if (errc != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("failed to open a stream: name -> {}, stream kind -> {}, error code -> {}", name, std::to_underlying(stream_kind), errc);
    return std::unexpected{errc};
  }

  auto* mem{kphp::memory::script::alloc(capacity)};
  kphp::log::assertion(mem != nullptr);
  kphp::log::debug("opened a stream: name -> {}, descriptor -> {}", name, descriptor);
  return std::expected<kphp::component::stream, int32_t>{stream{{static_cast<std::byte*>(mem), kphp::memory::script::free}, capacity, descriptor}};
}

inline auto stream::accept(size_t capacity, std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<std::optional<kphp::component::stream>> {
  const auto descriptor{co_await kphp::coro::io_scheduler::get().accept(timeout)};
  if (descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    kphp::log::warning("failed to accept a stream within a specified timeout: {}", timeout);
    co_return std::nullopt;
  }

  auto* mem{kphp::memory::script::alloc(capacity)};
  kphp::log::assertion(mem != nullptr);
  kphp::log::debug("accepted a stream: descriptor -> {}", descriptor);
  co_return stream{{static_cast<std::byte*>(mem), kphp::memory::script::free}, capacity, descriptor};
}

inline auto stream::clear() noexcept -> void {
  m_storage_size = 0;
}

inline auto stream::reset(k2::descriptor descriptor) noexcept -> void {
  if (descriptor == m_descriptor) [[unlikely]] {
    return;
  }

  clear();
  if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
    k2::free_descriptor(std::exchange(m_descriptor, descriptor));
  }
}

inline auto stream::size() const noexcept -> size_t {
  return m_storage_size;
}

inline auto stream::capacity() const noexcept -> size_t {
  return m_storage_capacity;
}

inline auto stream::reserve(size_t capacity) noexcept -> std::expected<void, int32_t> {
  if (capacity <= m_storage_capacity) [[unlikely]] {
    return std::expected<void, int32_t>{};
  }

  auto* cur_mem{m_storage.release()};
  auto* new_mem{static_cast<std::byte*>(kphp::memory::script::realloc(cur_mem, capacity))};
  if (new_mem == nullptr) [[unlikely]] {
    m_storage.reset(cur_mem);
    return std::unexpected{k2::errno_enomem};
  }
  m_storage.reset(new_mem);
  m_storage_capacity = capacity;
  return std::expected<void, int32_t>{};
}

inline auto stream::descriptor() const noexcept -> k2::descriptor {
  return m_descriptor;
}

inline auto stream::data() const noexcept -> std::span<const std::byte> {
  return {m_storage.get(), m_storage_size};
}

inline auto stream::read() noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  auto& io_scheduler{kphp::coro::io_scheduler::get()};

  for (;;) {
    const auto poll_status{co_await io_scheduler.poll(m_descriptor, kphp::coro::poll_op::read)};
    switch (poll_status) {
    case kphp::coro::poll_status::event:
      if (m_storage_capacity == m_storage_size) {
        if (auto expected{reserve(m_storage_capacity * 2)}; !expected) [[unlikely]] {
          co_return std::move(expected);
        }
      }
      [[likely]] m_storage_size += k2::read(m_descriptor, m_storage_capacity - m_storage_size, static_cast<void*>(std::next(m_storage.get(), m_storage_size)));
      break;
    case kphp::coro::poll_status::closed:
      [[likely]] co_return std::expected<void, int32_t>{};
    case kphp::coro::poll_status::error:
      co_return std::unexpected{k2::errno_efault};
    case kphp::coro::poll_status::timeout:
      co_return std::unexpected{k2::errno_ecanceled};
    }
  }
  std::unreachable();
}

inline auto stream::write(std::span<const std::byte> data) const noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  auto& io_scheduler{kphp::coro::io_scheduler::get()};

  size_t written{};
  for (; written < data.size();) {
    switch (co_await io_scheduler.poll(m_descriptor, kphp::coro::poll_op::write)) {
    case kphp::coro::poll_status::event:
      [[likely]] written += k2::write(m_descriptor, data.size() - written, static_cast<const void*>(std::next(data.data(), written)));
      break;
    case kphp::coro::poll_status::closed:
      co_return std::unexpected{k2::errno_eshutdown};
    case kphp::coro::poll_status::error:
      co_return std::unexpected{k2::errno_efault};
    case kphp::coro::poll_status::timeout:
      co_return std::unexpected{k2::errno_ecanceled};
    }
  }
  kphp::log::assertion(written == data.size());
  co_return std::expected<void, int32_t>{};
}

inline auto stream::shutdown_write() const noexcept -> void {
  k2::shutdown_write(m_descriptor);
}

} // namespace kphp::component
