// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::component {

class stream {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};

  explicit stream(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {}

public:
  stream(stream&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)) {}

  auto operator=(stream&& other) noexcept -> stream& {
    if (this != std::addressof(other)) {
      reset(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
    }
    return *this;
  }

  ~stream() {
    if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
      reset(k2::INVALID_PLATFORM_DESCRIPTOR);
    }
  }

  stream(const stream&) = delete;
  auto operator=(const stream&) -> stream& = delete;

  static auto open(std::string_view target, k2::stream_kind stream_kind) noexcept -> std::expected<kphp::component::stream, int32_t>;
  static auto accept(std::chrono::nanoseconds timeout = std::chrono::nanoseconds{0}) noexcept -> kphp::coro::task<std::optional<kphp::component::stream>>;

  auto descriptor() const noexcept -> k2::descriptor;
  auto reset(k2::descriptor descriptor) noexcept -> void;

  auto read(std::span<std::byte> buf) const noexcept -> kphp::coro::task<std::expected<size_t, int32_t>>;
  template<std::invocable<std::span<const std::byte>> F>
  auto read_all(F f) const noexcept -> kphp::coro::task<std::expected<void, int32_t>>;

  auto write(std::span<const std::byte> buf) const noexcept -> kphp::coro::task<std::expected<size_t, int32_t>>;
  auto write_all(std::span<const std::byte> buf) const noexcept -> kphp::coro::task<std::expected<void, int32_t>>;
  auto shutdown_write() noexcept -> void; // mark it non-const to indicate that it actually changes the stream's state
};

// ================================================================================================

inline auto stream::open(std::string_view target, k2::stream_kind stream_kind) noexcept -> std::expected<kphp::component::stream, int32_t> {
  int32_t errc{};
  k2::descriptor descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  switch (stream_kind) {
  case k2::stream_kind::component:
    errc = k2::open(std::addressof(descriptor), target.size(), target.data());
    break;
  case k2::stream_kind::tcp:
    errc = k2::tcp_connect(std::addressof(descriptor), target.data(), target.size());
    break;
  case k2::stream_kind::udp:
    errc = k2::udp_connect(std::addressof(descriptor), target.data(), target.size());
    break;
  }

  if (errc != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("failed to open a stream: name -> {}, stream kind -> {}, error code -> {}", target, std::to_underlying(stream_kind), errc);
    return std::unexpected{errc};
  }

  kphp::log::debug("opened a stream: name -> {}, descriptor -> {}", target, descriptor);
  return std::expected<kphp::component::stream, int32_t>{stream{descriptor}};
}

inline auto stream::accept(std::chrono::nanoseconds timeout) noexcept -> kphp::coro::task<std::optional<kphp::component::stream>> {
  const auto descriptor{co_await kphp::coro::io_scheduler::get().accept(timeout)};
  if (descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) [[unlikely]] {
    kphp::log::warning("failed to accept a stream within a specified timeout: {}", timeout);
    co_return std::nullopt;
  }

  kphp::log::debug("accepted a stream: descriptor -> {}", descriptor);
  co_return stream{descriptor};
}

inline auto stream::reset(k2::descriptor descriptor) noexcept -> void {
  if (descriptor == m_descriptor) [[unlikely]] {
    return;
  }
  k2::free_descriptor(std::exchange(m_descriptor, descriptor));
}

inline auto stream::descriptor() const noexcept -> k2::descriptor {
  return m_descriptor;
}

inline auto stream::read(std::span<std::byte> buf) const noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  for (size_t read{}; read < buf.size();) {
    switch (co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read)) {
    case kphp::coro::poll_status::event:
      [[likely]] read += k2::read(m_descriptor, buf.subspan(read));
      break;
    case kphp::coro::poll_status::closed:
      [[likely]] co_return std::expected<size_t, int32_t>{read};
    case kphp::coro::poll_status::error:
      co_return std::unexpected{k2::errno_efault};
    case kphp::coro::poll_status::timeout:
      co_return std::unexpected{k2::errno_ecanceled};
    }
  }
  co_return std::expected<size_t, int32_t>{buf.size()};
}

template<std::invocable<std::span<const std::byte>> F>
auto stream::read_all(F f) const noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  static constexpr size_t CHUNK_SIZE = 2048;
  std::array<std::byte, CHUNK_SIZE> chunk; // NOLINT

  for (;;) {
    auto expected{co_await read(chunk)};
    if (!expected || *expected == 0) {
      co_return expected.transform([](auto) noexcept {});
    }
    kphp::log::assertion(*expected <= CHUNK_SIZE);
    std::invoke(f, std::span<const std::byte>{chunk}.first(*expected));
  }
}

inline auto stream::write(std::span<const std::byte> buf) const noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  for (size_t written{}; written < buf.size();) {
    switch (co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::write)) {
    case kphp::coro::poll_status::event:
      [[likely]] written += k2::write(m_descriptor, buf.subspan(written));
      break;
    case kphp::coro::poll_status::closed:
      [[likely]] co_return std::expected<size_t, int32_t>{written};
    case kphp::coro::poll_status::error:
      co_return std::unexpected{k2::errno_efault};
    case kphp::coro::poll_status::timeout:
      co_return std::unexpected{k2::errno_ecanceled};
    }
  }
  co_return std::expected<size_t, int32_t>{buf.size()};
}

inline auto stream::write_all(std::span<const std::byte> buf) const noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  auto expected{co_await write(buf)};
  if (!expected) [[unlikely]] {
    co_return std::unexpected{expected.error()};
  }

  if (*expected != buf.size()) [[unlikely]] {
    co_return std::unexpected{k2::errno_eshutdown};
  }
  co_return std::expected<void, int32_t>{};
}

inline auto stream::shutdown_write() noexcept -> void { // NOLINT
  k2::shutdown_write(m_descriptor);
}

} // namespace kphp::component
