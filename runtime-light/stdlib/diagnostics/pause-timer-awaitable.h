// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>
#include <utility>

template<typename Awaiter, typename Timer>
class PauseTimerAwaiter final {
public:
  PauseTimerAwaiter(Awaiter awaiter, Timer& timer) noexcept
      : awaiter_{std::move(awaiter)},
        timer_{timer} {}

  bool await_ready() noexcept {
    return awaiter_.await_ready();
  }

  template<typename Coroutine>
  decltype(auto) await_suspend(Coroutine coroutine) noexcept {
    timer_.pause();
    return awaiter_.await_suspend(coroutine);
  }

  decltype(auto) await_resume() noexcept {
    timer_.resume();
    return awaiter_.await_resume();
  }

private:
  Awaiter awaiter_;
  Timer& timer_;
};

template<typename Awaitable, typename Timer>
class PauseTimerAwaitable final {
public:
  PauseTimerAwaitable(Awaitable awaitable, Timer& timer) noexcept
      : awaitable_{std::move(awaitable)},
        timer_{timer} {}

  auto operator co_await() noexcept {
    return PauseTimerAwaiter{awaitable_.operator co_await(), timer_};
  }

private:
  Awaitable awaitable_;
  Timer& timer_;
};

template<typename Timer, typename Awaitable>
auto pause_timer_while_awaiting(Timer& timer, Awaitable&& awaitable) noexcept {
  return PauseTimerAwaitable<std::remove_cvref_t<Awaitable>, Timer>{std::forward<Awaitable>(awaitable), timer};
}
