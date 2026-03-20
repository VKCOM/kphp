// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/when-any.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::component {

class watcher {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  // TODO it should watch for specific poll_op
  std::optional<kphp::coro::event> m_unwatch_event;

  explicit watcher(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {}

public:
  watcher(watcher&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)),
        m_unwatch_event(std::exchange(other.m_unwatch_event, {})) {}

  watcher& operator=(watcher&& other) noexcept {
    if (this != std::addressof(other)) {
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_unwatch_event = std::exchange(other.m_unwatch_event, {});
    }
    return *this;
  }

  watcher() = delete;
  watcher(const watcher&) = delete;
  watcher& operator=(const watcher&) = delete;

  ~watcher() {
    unwatch();
  }

  static auto create(k2::descriptor descriptor) noexcept -> std::expected<watcher, int32_t>;

  template<std::invocable on_event_handler_type>
  auto watch(on_event_handler_type&& f) noexcept -> std::expected<void, int32_t>;

  auto unwatch() noexcept -> void;
};

inline auto watcher::create(k2::descriptor descriptor) noexcept -> std::expected<watcher, int32_t> {
  if (descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    return std::unexpected{k2::errno_einval};
  }
  return watcher{descriptor};
}

template<std::invocable on_event_handler_type>
auto watcher::watch(on_event_handler_type&& f) noexcept -> std::expected<void, int32_t> {
  if (m_unwatch_event) { // already watching
    return std::unexpected{k2::errno_ealready};
  }

  k2::StreamStatus stream_status{};
  k2::stream_status(m_descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    return std::unexpected{stream_status.libc_errno};
  }

  static constexpr auto watcher{[](k2::descriptor descriptor, kphp::coro::event& unwatch_event, on_event_handler_type f) noexcept -> kphp::coro::task<> {
    static constexpr auto unwatch_awaiter{[](kphp::coro::event& unwatch_event) noexcept -> kphp::coro::task<> { co_await unwatch_event; }};
    static constexpr auto update_awaiter{[](k2::descriptor descriptor) noexcept -> kphp::coro::task<std::monostate> {
      k2::StreamStatus stream_status{};
      auto& io_scheduler{kphp::coro::io_scheduler::get()};
      for (;;) {
        k2::stream_status(descriptor, std::addressof(stream_status));
        if (stream_status.write_status == k2::IOStatus::IOClosed) {
          co_return std::monostate{};
        }

        using namespace std::chrono_literals;
        co_await io_scheduler.schedule(150ms);
      }
    }};

    if (std::holds_alternative<std::monostate>(co_await kphp::coro::when_any(update_awaiter(descriptor), unwatch_awaiter(unwatch_event)))) {
      if constexpr (kphp::coro::is_async_function_v<on_event_handler_type>) {
        co_await std::invoke(std::move(f));
      } else {
        std::invoke(std::move(f));
      }
    }
  }};

  if (!kphp::coro::io_scheduler::get().start(watcher(m_descriptor, m_unwatch_event.emplace(), std::forward<on_event_handler_type>(f)))) {
    m_unwatch_event.reset();
    return std::unexpected{k2::errno_ebusy};
  }
  return {};
}

inline auto watcher::unwatch() noexcept -> void {
  if (!m_unwatch_event) {
    return;
  }
  m_unwatch_event->set();
  m_unwatch_event.reset();
}

} // namespace kphp::component
