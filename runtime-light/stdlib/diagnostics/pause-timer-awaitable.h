// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

template<typename Awaiter, typename Timer>
class PauseTimerAwaiter final {
public:
  template<typename A>
  PauseTimerAwaiter(A&& awaiter, Timer* timer) noexcept
      : awaiter_{std::forward<A>(awaiter)},
        timer_{timer} {}

  template<typename Awaitable>
  PauseTimerAwaiter(Awaitable& awaitable, Timer* timer, std::in_place_t) noexcept
      : awaiter_{awaitable.operator co_await()},
        timer_{timer} {}

  bool await_ready() noexcept {
    return awaiter_.await_ready();
  }

  template<typename Coroutine>
  decltype(auto) await_suspend(Coroutine coroutine) noexcept {
    if (timer_ != nullptr) {
      should_resume_ = timer_->pause();
    }
    return awaiter_.await_suspend(coroutine);
  }

  decltype(auto) await_resume() noexcept {
    if (should_resume_) {
      timer_->resume();
      should_resume_ = false;
    }
    return awaiter_.await_resume();
  }

  ~PauseTimerAwaiter() {
    if (should_resume_) {
      timer_->resume();
    }
  }

private:
  Awaiter awaiter_;
  Timer* timer_;
  bool should_resume_{false};
};

template<typename Awaiter, typename Timer>
PauseTimerAwaiter(Awaiter&&, Timer*) -> PauseTimerAwaiter<std::remove_cvref_t<Awaiter>, Timer>;

template<typename Awaitable, typename Timer>
class PauseTimerAwaitable final {
public:
  template<typename A>
  PauseTimerAwaitable(A&& awaitable, Timer* timer) noexcept
      : awaitable_{std::forward<A>(awaitable)},
        timer_{timer} {}

  auto operator co_await() noexcept {
    using awaiter_type = decltype(awaitable_.operator co_await());
    return PauseTimerAwaiter<awaiter_type, Timer>{awaitable_, timer_, std::in_place};
  }

private:
  Awaitable awaitable_;
  Timer* timer_;
};

template<typename Timer, typename Awaitable>
auto pause_timer_while_awaiting(Timer& timer, Awaitable&& awaitable) noexcept {
  // Wrapping a lazy task excludes its whole body, including synchronous work.
  // For I/O, use this at the leaf that parks the coroutine, not around poll().
  return PauseTimerAwaitable<Awaitable, Timer>{std::forward<Awaitable>(awaitable), std::addressof(timer)};
}

template<typename Timer, typename Awaitable>
auto pause_timer_while_awaiting(Timer* timer, Awaitable&& awaitable) noexcept {
  return PauseTimerAwaitable<Awaitable, Timer>{std::forward<Awaitable>(awaitable), timer};
}
