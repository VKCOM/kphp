// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/control-functions.h"
#include "runtime-light/coroutine/detail/allocator/coroutine-malloc-interface.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"
#include "runtime-light/coroutine/task-allocator-guard.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/metaprogramming/type-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::when_any {

class when_any_latch {
  bool m_toggled{};
  std::coroutine_handle<> m_awaiting_coroutine;
  kphp::coro::detail::memory::task_allocator& m_task_allocator{kphp::coro::detail::memory::task_allocator::get()};

public:
  when_any_latch() noexcept = default;

  when_any_latch(when_any_latch&& other) noexcept
      : m_toggled(std::exchange(other.m_toggled, false)),
        m_awaiting_coroutine(std::exchange(other.m_awaiting_coroutine, {})),
        m_task_allocator(other.m_task_allocator) {}

  auto operator=(when_any_latch&& other) noexcept -> when_any_latch& {
    if (this != std::addressof(other)) {
      m_toggled = std::exchange(other.m_toggled, false);
      m_awaiting_coroutine = std::exchange(other.m_awaiting_coroutine, {});
    }
    return *this;
  }

  ~when_any_latch() = default;

  when_any_latch(const when_any_latch&) = delete;
  when_any_latch& operator=(const when_any_latch&) = delete;

  auto is_ready() const noexcept -> bool {
    return m_toggled;
  }

  auto try_await(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    return !m_toggled;
  }

  auto notify_awaitable_completed() noexcept -> void {
    m_toggled = true;
    if (m_awaiting_coroutine != nullptr) {
      kphp::coro::resume(m_awaiting_coroutine, m_task_allocator);
    }
  }

  auto task_allocator() noexcept -> kphp::coro::detail::memory::task_allocator& {
    return m_task_allocator;
  }
};

template<typename task_container_type>
class when_any_ready_awaitable;

template<>
class when_any_ready_awaitable<std::tuple<>> {
public:
  constexpr when_any_ready_awaitable() noexcept = default;
  explicit constexpr when_any_ready_awaitable(std::tuple<> /*unused*/) noexcept {}

  constexpr auto await_ready() const noexcept -> bool {
    return true;
  }

  constexpr auto await_suspend(std::coroutine_handle<> /*unused*/) noexcept -> void {}

  constexpr auto await_resume() const noexcept -> std::variant<std::monostate> {
    return {};
  }
};

template<typename... task_types>
class when_any_ready_awaitable<std::tuple<task_types...>> {
  when_any_latch m_latch;
  std::tuple<task_types...> m_tasks;
  std::optional<std::variant<typename task_types::result_type...>> m_result;

  struct awaiter : private kphp::coro::task_allocator_guard {
    bool m_started{};
    when_any_ready_awaitable& m_awaitable;
    kphp::coro::async_stack_frame* m_caller_async_stack_frame{};

    explicit awaiter(when_any_ready_awaitable& awaitable) noexcept
        : kphp::coro::task_allocator_guard(awaitable.m_latch.task_allocator()),
          m_awaitable(awaitable) {}

    auto await_ready() noexcept -> bool {
      kphp::log::assertion(!std::exchange(m_started, true)); // to make sure it's not co_awaited more than once
      return m_awaitable.m_latch.is_ready();
    }

    template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
    [[clang::noinline]] auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> bool {
      // async stack frame handling
      void* const return_address{STACK_RETURN_ADDRESS};
      m_caller_async_stack_frame = std::addressof(awaiting_coroutine.promise().get_async_stack_frame());

      std::apply([&latch = m_awaitable.m_latch, &caller_async_stack_frame = *m_caller_async_stack_frame,
                  return_address](auto&... tasks) noexcept { (tasks.start(latch, caller_async_stack_frame, return_address), ...); },
                 m_awaitable.m_tasks);
      return m_awaitable.m_latch.try_await(awaiting_coroutine);
    }

    auto await_resume() noexcept {
      // restore caller's async_stack_frame unless it's not set which could happen in case no suspension occured
      if (m_caller_async_stack_frame != nullptr) {
        kphp::log::assertion(m_caller_async_stack_frame->async_stack_root != nullptr);
        m_caller_async_stack_frame->async_stack_root->top_async_stack_frame = m_caller_async_stack_frame;
      }

      const auto task_result_processor{[&result = m_awaitable.m_result](auto&& task) noexcept {
        if (auto task_result{std::forward<decltype(task)>(task).result()}; !result.has_value() && task_result.has_value()) {
          using result_variant_type = std::remove_cvref<decltype(result)>::type::value_type;
          using task_result_type = decltype(task_result)::value_type;
          result =
              result_variant_type{std::in_place_index<kphp::type_functions::variant_index<result_variant_type, task_result_type>()>, *std::move(task_result)};
        }
      }};
      std::apply(
          [&task_result_processor, &latch = m_awaitable.m_latch](auto&&... tasks) noexcept {
            (std::invoke(task_result_processor, std::forward<decltype(tasks)>(tasks)), ...);
            (tasks.reset(latch.task_allocator()), ...);
          },
          std::move(m_awaitable.m_tasks));
      kphp::log::assertion(m_awaitable.m_result.has_value());

      return std::move(*m_awaitable.m_result);
    }
  };

public:
  explicit when_any_ready_awaitable(task_types&&... tasks) noexcept
      : m_tasks(std::move(tasks)...) {}

  when_any_ready_awaitable(when_any_ready_awaitable&& other) noexcept
      : m_tasks(std::move(other.m_tasks)) {}

  ~when_any_ready_awaitable() = default;

  when_any_ready_awaitable(const when_any_ready_awaitable&) = delete;
  when_any_ready_awaitable& operator=(const when_any_ready_awaitable&) = delete;
  when_any_ready_awaitable& operator=(when_any_ready_awaitable&&) = delete;

  auto operator co_await() && noexcept {
    return awaiter{*this};
  }
};

template<typename return_type, typename promise_type>
class when_any_task_promise_base : public kphp::coro::async_stack_element {
  when_any_latch* m_latch{};

public:
  when_any_task_promise_base() noexcept = default;

  template<typename... Args>
  auto operator new(size_t n, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::coro::detail::memory::alloc(n);
  }

  template<typename... Args>
  auto operator new(size_t n, std::align_val_t al, [[maybe_unused]] Args&&... args) noexcept -> void* {
    return kphp::coro::detail::memory::alloc_aligned(n, al);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::coro::detail::memory::free(ptr);
  }

  auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  auto final_suspend() const noexcept {
    struct completion_notifier {
      constexpr auto await_ready() const noexcept {
        return false;
      }

      auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> void {
        coroutine.promise().m_latch->notify_awaitable_completed();
      }

      constexpr auto await_resume() const noexcept -> void {}
    };
    return completion_notifier{};
  }

  auto unhandled_exception() const noexcept -> void {
    kphp::log::error("internal unhandled exception");
  }

  auto start(when_any_latch& latch, kphp::coro::async_stack_frame& caller_async_stack_frame, void* return_address) noexcept {
    m_latch = std::addressof(latch);

    auto& async_stack_frame{get_async_stack_frame()};
    kphp::log::assertion(caller_async_stack_frame.async_stack_root != nullptr);
    // initialize when_all_task's async stack frame and make it the top frame
    async_stack_frame.caller_async_stack_frame = std::addressof(caller_async_stack_frame);
    async_stack_frame.async_stack_root = caller_async_stack_frame.async_stack_root;
    async_stack_frame.return_address = return_address;
    async_stack_frame.async_stack_root->top_async_stack_frame = std::addressof(async_stack_frame);
    kphp::coro::resume(std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this)), m_latch->task_allocator());
  }
};

template<typename return_type>
class when_any_task {
  template<std::same_as<return_type> T>
  struct when_any_task_promise_non_void;
  struct when_any_task_promise_void;

public:
  using result_type = std::conditional_t<std::is_void_v<return_type>, kphp::coro::void_value, std::remove_cvref_t<return_type>>;
  using promise_type = std::conditional_t<std::is_void_v<return_type>, when_any_task_promise_void, when_any_task_promise_non_void<return_type>>;

private:
  std::coroutine_handle<promise_type> m_coroutine;

  struct when_any_task_promise_common : public when_any_task_promise_base<return_type, promise_type> {
    auto get_return_object() noexcept {
      return when_any_task{std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this))};
    }

    static auto get_return_object_on_allocation_failure() noexcept -> when_any_task {
      kphp::log::error("cannot allocate memory for when_any_task");
    }

    auto return_void() const noexcept -> void {
      kphp::log::assertion(false); // we should have suspended at co_yield
    }
  };

  template<std::same_as<return_type> T>
  struct when_any_task_promise_non_void : public when_any_task_promise_common {
  private:
    std::optional<result_type> m_result;

  public:
    auto yield_value(return_type&& return_value) noexcept {
      m_result.emplace(std::move(return_value));
      return when_any_task_promise_common::final_suspend();
    }

    auto result() noexcept -> std::optional<result_type> {
      return std::move(m_result);
    }
  };

  struct when_any_task_promise_void : public when_any_task_promise_common {
  private:
    std::optional<kphp::coro::void_value> m_result;

  public:
    auto yield_value(kphp::coro::void_value&& return_value) noexcept {
      m_result.emplace(return_value);
      return when_any_task_promise_common::final_suspend();
    }

    auto result() noexcept -> std::optional<kphp::coro::void_value> {
      return m_result;
    }

    constexpr auto return_void() const noexcept -> void {}
  };

  auto start(when_any_latch& latch, kphp::coro::async_stack_frame& caller_async_stack_frame, void* return_address) noexcept -> void {
    if (!latch.is_ready()) [[likely]] {
      m_coroutine.promise().start(latch, caller_async_stack_frame, return_address);
    }
  }

public:
  // to be able to call start()
  template<typename task_container_type>
  friend class when_any_ready_awaitable;

  explicit when_any_task(std::coroutine_handle<promise_type> coroutine) noexcept
      : m_coroutine(coroutine) {}

  when_any_task(when_any_task&& other) noexcept
      : m_coroutine(std::exchange(other.m_coroutine, {})) {}

  ~when_any_task() {
    if (m_coroutine != nullptr) {
      kphp::coro::destroy(m_coroutine, kphp::coro::detail::memory::task_allocator::get());
    }
  }

  when_any_task(const when_any_task&) = delete;
  when_any_task& operator=(const when_any_task&) = delete;
  when_any_task& operator=(when_any_task&&) = delete;

  auto result() && noexcept {
    return m_coroutine.promise().result();
  }

  auto reset(kphp::coro::detail::memory::task_allocator& task_allocator) noexcept -> void {
    if (m_coroutine != nullptr) {
      kphp::coro::destroy(std::exchange(m_coroutine, nullptr), task_allocator);
    }
  }
};

template<kphp::coro::concepts::awaitable awaitable_type>
auto make_when_any_task(awaitable_type awaitable) noexcept -> when_any_task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(awaitable);
    co_yield kphp::coro::void_value{};
  } else {
    co_yield co_await std::move(awaitable);
  }
}

template<typename F, typename... Args>
requires(kphp::coro::is_task_function_v<F, Args...>)
auto make_when_any_task(F f,
                        Args... args) noexcept -> when_any_task<typename kphp::coro::awaitable_traits<std::invoke_result_t<F, Args...>>::awaiter_return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<std::invoke_result_t<F, Args...>>::awaiter_return_type>) {
    co_await kphp::coro::on_stack(std::move(f), std::move(args)...);
    co_yield kphp::coro::void_value{};
  } else {
    co_yield co_await kphp::coro::on_stack(std::move(f), std::move(args)...);
  }
}

} // namespace kphp::coro::detail::when_any
