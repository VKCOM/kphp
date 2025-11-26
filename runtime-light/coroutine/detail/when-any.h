// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/coroutine/async-stack-methods.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/concepts.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/coroutine/void-value.h"
#include "runtime-light/metaprogramming/type-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::when_any {

class when_any_latch {
  bool m_toggled{};
  std::coroutine_handle<> m_awaiting_coroutine;

public:
  when_any_latch() noexcept = default;

  when_any_latch(when_any_latch&& other) noexcept
      : m_toggled(std::exchange(other.m_toggled, false)),
        m_awaiting_coroutine(std::exchange(other.m_awaiting_coroutine, {})) {}

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
      kphp::coro::resume(m_awaiting_coroutine);
    }
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

  struct awaiter {
    bool m_started{};
    when_any_ready_awaitable& m_awaitable;
    kphp::coro::async_stack_frame* m_caller_async_stack_frame{};

    explicit awaiter(when_any_ready_awaitable& awaitable) noexcept
        : m_awaitable(awaitable) {}

    auto await_ready() noexcept -> bool {
      kphp::log::assertion(!std::exchange(m_started, true)); // to make sure it's not co_awaited more than once
      return m_awaitable.m_latch.is_ready();
    }

    template<std::derived_from<kphp::coro::async_stack_element> caller_promise_type>
    [[clang::noinline]] auto await_suspend(std::coroutine_handle<caller_promise_type> awaiting_coroutine) noexcept -> bool {
      // async stack frame handling
      void* const return_address{STACK_RETURN_ADDRESS};
      m_caller_async_stack_frame = std::addressof(awaiting_coroutine.promise().get_async_stack_frame());

      std::apply([&latch = m_awaitable.m_latch, return_address](auto&... tasks) noexcept { (tasks.start(latch, return_address), ...); }, m_awaitable.m_tasks);
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
      std::apply([&task_result_processor](auto&&... tasks) noexcept { (std::invoke(task_result_processor, std::forward<decltype(tasks)>(tasks)), ...); },
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
    return kphp::memory::script::alloc(n);
  }

  auto operator delete(void* ptr, [[maybe_unused]] size_t n) noexcept -> void {
    kphp::memory::script::free(ptr);
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

  auto start(when_any_latch& latch, void* return_address) noexcept {
    m_latch = std::addressof(latch);

    auto& async_stack_frame{get_async_stack_frame()};
    // initialize when_all_task's async stack frame and make it the top frame
    async_stack_frame.caller_async_stack_frame = nullptr;
    async_stack_frame.async_stack_root = CoroutineInstanceState::get_next_root();
    async_stack_frame.return_address = return_address;
    async_stack_frame.async_stack_root->top_async_stack_frame = std::addressof(async_stack_frame);

    decltype(auto) handle = std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type*>(this));
    kphp::coro::resume(handle, async_stack_frame.async_stack_root);
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
    auto yield_value() const noexcept {
      return when_any_task_promise_common::final_suspend();
    }

    constexpr auto result() const noexcept -> std::optional<kphp::coro::void_value> {
      return std::optional<kphp::coro::void_value>{std::in_place};
    }

    constexpr auto return_void() const noexcept -> void {}
  };

  auto start(when_any_latch& latch, void* return_address) noexcept -> void {
    if (!latch.is_ready()) [[likely]] {
      m_coroutine.promise().start(latch, return_address);
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
      m_coroutine.destroy();
    }
  }

  when_any_task(const when_any_task&) = delete;
  when_any_task& operator=(const when_any_task&) = delete;
  when_any_task& operator=(when_any_task&&) = delete;

  auto result() && noexcept {
    return m_coroutine.promise().result();
  }
};

template<kphp::coro::concepts::awaitable awaitable_type>
auto make_when_any_task(awaitable_type awaitable) noexcept -> when_any_task<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type> {
  if constexpr (std::is_void_v<typename kphp::coro::awaitable_traits<awaitable_type>::awaiter_return_type>) {
    co_await std::move(awaitable);
  } else {
    co_yield co_await std::move(awaitable);
  }
}

} // namespace kphp::coro::detail::when_any
