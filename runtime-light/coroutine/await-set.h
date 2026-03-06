// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

template<typename return_type>
class await_set {
  std::unique_ptr<detail::await_set::await_broker<return_type>> m_await_broker;
  kphp::coro::async_stack_root& m_coroutine_stack_root;

public:
  await_set() noexcept
      : m_await_broker(std::make_unique<detail::await_set::await_broker<return_type>>()),
        m_coroutine_stack_root(CoroutineInstanceState::get().coroutine_stack_root) {}

  await_set(await_set&& other) noexcept
      : m_await_broker(std::move(other.m_await_broker)),
        m_coroutine_stack_root(other.m_coroutine_stack_root) {}

  await_set& operator=(await_set&& other) noexcept {
    if (this != std::addressof(other)) {
      m_await_broker = std::move(other.m_await_broker);
      m_coroutine_stack_root = other.m_coroutine_stack_root;
    }
    return *this;
  }

  await_set(const await_set&) = delete;
  await_set& operator=(const await_set&) = delete;

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  void push(awaitable_type awaitable) noexcept {
    kphp::log::assertion(m_await_broker != nullptr);
    m_await_broker->start_task(detail::await_set::make_await_set_task(std::move(awaitable)), m_coroutine_stack_root, STACK_RETURN_ADDRESS);
  }

  auto next() noexcept {
    kphp::log::assertion(m_await_broker != nullptr);
    return detail::await_set::await_set_awaitable<return_type>{*m_await_broker};
  }

  auto try_next() noexcept {
    using result_type = std::optional<decltype(std::declval<detail::await_set::await_set_task<return_type>>().result())>;
    if (m_await_broker == nullptr) [[unlikely]] {
      return result_type{std::nullopt};
    }
    return result_type{m_await_broker->try_get_result()};
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t size() const noexcept {
    kphp::log::assertion(m_await_broker != nullptr);
    return m_await_broker->size();
  }
};

} // namespace kphp::coro
