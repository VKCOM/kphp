// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <optional>
#include <type_traits>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/detail/await-set.h"
#include "runtime-light/coroutine/type-traits.h"

namespace kphp::coro {

template<typename return_type>
class await_set {
  detail::await_set::await_broker<return_type> m_await_broker;
  kphp::coro::async_stack_root& m_coroutine_stack_root;

public:
  await_set() noexcept
      : m_coroutine_stack_root(CoroutineInstanceState::get().coroutine_stack_root) {}

  await_set(const await_set&) = delete;
  await_set(await_set&& other) = delete;

  await_set& operator=(const await_set&) = delete;
  await_set& operator=(await_set&& other) = delete;

  ~await_set() = default;

  template<typename... Args>
  void* operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept {
    return kphp::memory::script::alloc(n);
  }

  template<typename... Args>
  auto operator new(size_t n, std::align_val_t al, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::memory::script::alloc_aligned(n, al);
  }

  void operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept {
    kphp::memory::script::free(ptr);
  }

  template<typename awaitable_type>
  requires kphp::coro::concepts::awaitable<awaitable_type> && std::is_same_v<typename awaitable_traits<awaitable_type>::awaiter_return_type, return_type>
  void push(awaitable_type awaitable) noexcept {
    m_await_broker.start_task(detail::await_set::make_await_set_task(std::move(awaitable)), m_coroutine_stack_root, STACK_RETURN_ADDRESS);
  }

  auto next() noexcept {
    return detail::await_set::await_set_awaitable<return_type>{m_await_broker};
  }

  auto try_next() noexcept {
    using result_type = std::optional<decltype(std::declval<typename detail::await_set::await_set_task<return_type>::promise_type>().result())>;
    return result_type{m_await_broker.try_get_result()};
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t size() const noexcept {
    return m_await_broker.size();
  }
};

} // namespace kphp::coro
