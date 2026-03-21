// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/streams/watcher.h"

namespace kphp::component {

class connection {
  kphp::component::stream m_stream;
  kphp::component::watcher m_watcher;
  uint32_t m_ignore_abort_level{};

  connection(kphp::component::stream&& stream, kphp::component::watcher&& watcher) noexcept
      : m_stream(std::move(stream)),
        m_watcher(std::move(watcher)) {}

public:
  connection() = delete;
  connection(const connection&) = delete;
  auto operator=(const connection&) = delete;

  connection(connection&&) noexcept = default;
  auto operator=(connection&&) noexcept -> connection& = default;
  ~connection() = default;

  static auto from_stream(kphp::component::stream&& stream) noexcept -> std::expected<connection, int32_t>;

  auto get_stream() noexcept -> kphp::component::stream&;

  auto increase_ignore_abort_level() noexcept -> void;
  auto decrease_ignore_abort_level() noexcept -> void;
  auto get_ignore_abort_level() const noexcept -> uint32_t;

  auto is_aborted() const noexcept -> bool;

  template<std::invocable on_abort_handler_type>
  auto register_abort_handler(on_abort_handler_type&& h) noexcept -> std::expected<void, int32_t>;
  auto unregister_abort_handler() noexcept -> void;
};

// ================================================================================================

inline auto connection::from_stream(kphp::component::stream&& stream) noexcept -> std::expected<connection, int32_t> {
  k2::StreamStatus stream_status{};
  k2::stream_status(stream.descriptor(), std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok || stream_status.write_status == k2::IOStatus::IOClosed) [[unlikely]] {
    return std::unexpected{stream_status.libc_errno != k2::errno_ok ? stream_status.libc_errno : k2::errno_eshutdown};
  }

  auto expected_watcher{kphp::component::watcher::create(stream.descriptor())};
  if (!expected_watcher) [[unlikely]] {
    return std::unexpected{expected_watcher.error()};
  }

  auto watcher{*std::move(expected_watcher)};
  // watcher.watch(std::forward<on_close_handler_type>(h));
  return connection{std::move(stream), std::move(watcher)};
}

inline auto connection::get_stream() noexcept -> kphp::component::stream& {
  return m_stream;
}

inline auto connection::increase_ignore_abort_level() noexcept -> void {
  ++m_ignore_abort_level;
}

inline auto connection::decrease_ignore_abort_level() noexcept -> void {
  if (m_ignore_abort_level > 0) {
    --m_ignore_abort_level;
  }
}

inline auto connection::get_ignore_abort_level() const noexcept -> uint32_t {
  return m_ignore_abort_level;
}

inline auto connection::is_aborted() const noexcept -> bool {
  return m_stream.status().write_status == k2::IOStatus::IOClosed;
}

template<std::invocable on_abort_handler_type>
auto connection::register_abort_handler(on_abort_handler_type&& h) noexcept -> std::expected<void, int32_t> {
  return m_watcher.watch(std::forward<on_abort_handler_type>(h));
}

inline auto connection::unregister_abort_handler() noexcept -> void {
  m_watcher.unwatch();
}
} // namespace kphp::component
