// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"

namespace kphp::component {

class connection {
  struct shared_state : refcountable_php_classes<shared_state> {
    std::optional<kphp::coro::event> m_unwatch_event;

    shared_state() noexcept = default;
    shared_state(shared_state&&) noexcept = default;
    shared_state& operator=(shared_state&&) noexcept = default;
    ~shared_state() = default;

    shared_state(const shared_state&) = delete;
    shared_state operator=(const shared_state&) = delete;
  };

  class_instance<shared_state> m_shared_state;
  kphp::component::stream m_stream;
  uint32_t m_ignore_abort_level{};

  explicit connection(kphp::component::stream&& stream) noexcept;

public:
  connection() = delete;
  connection(const connection&) = delete;
  auto operator=(const connection&) = delete;

  connection(connection&&) noexcept = default;
  auto operator=(connection&& other) noexcept -> connection&;

  ~connection() {
    unregister_abort_handler();
  }

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

inline connection::connection(kphp::component::stream&& stream) noexcept
    : m_stream(std::move(stream)) {
  kphp::log::assertion(!m_shared_state.alloc().is_null());
}

inline auto connection::operator=(connection&& other) noexcept -> connection& {
  if (this != std::addressof(other)) {
    unregister_abort_handler();
    m_shared_state = std::move(other.m_shared_state);
    m_stream = std::move(other.m_stream);
    m_ignore_abort_level = other.m_ignore_abort_level;
  }
  return *this;
}

inline auto connection::from_stream(kphp::component::stream&& stream) noexcept -> std::expected<connection, int32_t> {
  if (const auto status{stream.status()}; status.libc_errno != k2::errno_ok) [[unlikely]] {
    return std::unexpected{status.libc_errno};
  }
  return connection{std::move(stream)};
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
  if (m_shared_state.is_null()) [[unlikely]] {
    return std::unexpected{k2::errno_eshutdown};
  }
  if (m_shared_state.get()->m_unwatch_event.has_value()) [[unlikely]] { // already registered
    return std::unexpected{k2::errno_ealready};
  }

  if (const auto status{m_stream.status()}; status.libc_errno != k2::errno_ok || status.write_status == k2::IOStatus::IOClosed) [[unlikely]] {
    return std::unexpected{status.libc_errno != k2::errno_ok ? status.libc_errno : k2::errno_ecanceled};
  }

  static constexpr auto watcher{[](k2::descriptor descriptor, class_instance<shared_state> state, on_abort_handler_type h) noexcept -> kphp::coro::task<> {
    static constexpr auto unwatch_awaiter{[](class_instance<shared_state> state) noexcept -> kphp::coro::task<> {
      kphp::log::assertion(state.get()->m_unwatch_event.has_value());
      co_await *state.get()->m_unwatch_event;
    }};

    static constexpr auto descriptor_awaiter{[](k2::descriptor descriptor) noexcept -> kphp::coro::task<std::monostate> {
      k2::StreamStatus stream_status{};
      auto& io_scheduler{kphp::coro::io_scheduler::get()};
      for (;;) { // FIXME it should actually use scheduler.poll
        k2::stream_status(descriptor, std::addressof(stream_status));
        if (stream_status.write_status == k2::IOStatus::IOClosed) {
          co_return std::monostate{};
        }

        using namespace std::chrono_literals;
        co_await io_scheduler.schedule(150ms);
      }
    }};

    kphp::log::assertion(!state.is_null());
    if (!state.get()->m_unwatch_event.has_value()) { // already unregistered
      co_return;
    }

    const auto finalizer{vk::finally([state] noexcept { state.get()->m_unwatch_event.reset(); })};
    const auto v{co_await kphp::coro::when_any(unwatch_awaiter(std::move(state)), descriptor_awaiter(descriptor))};
    if (std::holds_alternative<std::monostate>(v)) {
      if constexpr (kphp::coro::is_async_function_v<on_abort_handler_type>) {
        co_await std::invoke(std::move(h));
      } else {
        std::invoke(std::move(h));
      }
    }
  }};

  m_shared_state.get()->m_unwatch_event.emplace();
  if (!kphp::coro::io_scheduler::get().spawn(watcher(m_stream.descriptor(), m_shared_state, std::forward<on_abort_handler_type>(h)))) [[unlikely]] {
    m_shared_state.get()->m_unwatch_event.reset();
    return std::unexpected{k2::errno_ebusy};
  }
  return {};
}

inline auto connection::unregister_abort_handler() noexcept -> void {
  if (m_shared_state.is_null() || !m_shared_state.get()->m_unwatch_event.has_value()) {
    return;
  }
  m_shared_state.get()->m_unwatch_event->set();
  m_shared_state.get()->m_unwatch_event.reset();
}

} // namespace kphp::component
