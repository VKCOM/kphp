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

#include "common/containers/final_action.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/when-any.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::component {

class watcher {
  struct shared_state : refcountable_php_classes<shared_state> {
    std::optional<kphp::coro::event> m_unwatch_event;

    shared_state() noexcept = default;
    shared_state(shared_state&&) noexcept = default;
    shared_state& operator=(shared_state&&) noexcept = default;
    ~shared_state() = default;

    shared_state(const shared_state&) = delete;
    shared_state operator=(const shared_state&) = delete;
  };

  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  class_instance<shared_state> m_shared_state;

  explicit watcher(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {
    kphp::log::assertion(!m_shared_state.alloc().is_null());
  }

public:
  watcher(watcher&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)),
        m_shared_state(std::move(other.m_shared_state)) {}

  watcher& operator=(watcher&& other) noexcept {
    if (this != std::addressof(other)) {
      unwatch();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_shared_state = std::move(other.m_shared_state);
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
  if (m_shared_state.is_null()) [[unlikely]] {
    return std::unexpected{k2::errno_eshutdown};
  }
  if (m_shared_state.get()->m_unwatch_event.has_value()) { // already watching
    return std::unexpected{k2::errno_ealready};
  }

  k2::StreamStatus stream_status{};
  k2::stream_status(m_descriptor, std::addressof(stream_status));
  if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
    return std::unexpected{stream_status.libc_errno};
  }

  static constexpr auto watcher{[](k2::descriptor descriptor, class_instance<shared_state> state, on_event_handler_type f) noexcept -> kphp::coro::task<> {
    kphp::log::assertion(!state.is_null() && state.get()->m_unwatch_event.has_value());
    const auto finalizer{vk::finally([state] noexcept { state.get()->m_unwatch_event.reset(); })};

    static constexpr auto unwatch_awaiter{[](class_instance<shared_state> state) noexcept -> kphp::coro::task<> { co_await *state.get()->m_unwatch_event; }};

    static constexpr auto descriptor_awaiter{[](k2::descriptor descriptor) noexcept -> kphp::coro::task<std::monostate> {
      k2::StreamStatus stream_status{};
      auto& io_scheduler{kphp::coro::io_scheduler::get()};
      for (;;) { // TODO it should watch for specific poll_op
        k2::stream_status(descriptor, std::addressof(stream_status));
        if (stream_status.write_status == k2::IOStatus::IOClosed) {
          co_return std::monostate{};
        }

        using namespace std::chrono_literals;
        co_await io_scheduler.schedule(150ms);
      }
    }};

    const auto v{co_await kphp::coro::when_any(descriptor_awaiter(descriptor), unwatch_awaiter(std::move(state)))};
    if (std::holds_alternative<std::monostate>(v)) {
      if constexpr (kphp::coro::is_async_function_v<on_event_handler_type>) {
        co_await std::invoke(std::move(f));
      } else {
        std::invoke(std::move(f));
      }
    }
  }};

  m_shared_state.get()->m_unwatch_event.emplace();
  if (!kphp::coro::io_scheduler::get().spawn(watcher(m_descriptor, m_shared_state, std::forward<on_event_handler_type>(f)))) {
    m_shared_state.get()->m_unwatch_event.reset();
    return std::unexpected{k2::errno_ebusy};
  }
  return {};
}

inline auto watcher::unwatch() noexcept -> void {
  if (m_shared_state.is_null() || !m_shared_state.get()->m_unwatch_event.has_value()) {
    return;
  }
  m_shared_state.get()->m_unwatch_event->set();
  m_shared_state.get()->m_unwatch_event.reset();
}

} // namespace kphp::component
