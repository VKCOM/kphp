#pragma once

#include <coroutine>

#include "runtime-light/component/component.h"
#include "runtime-light/utils/logs.h"

struct platform_switch_t {
  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    get_component_context()->poll_status = PollStatus::PollBlocked;
    php_debug("suspend on stream %lu", get_component_context()->awaited_stream);
    get_component_context()->suspend_point = h;
  }

  constexpr void await_resume() const noexcept {}
};

struct test_yield_t {
  bool await_ready() const noexcept {
    return !get_platform_context()->please_yield.load();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    get_component_context()->poll_status = PollStatus::PollReschedule;
    php_debug("suspend on yield");
    get_component_context()->suspend_point = h;
  }

  constexpr void await_resume() const noexcept {}
};

struct wait_input_query_t {
  bool await_ready() const noexcept {
    return !get_component_context()->pending_queries.empty();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    php_assert(get_component_context()->standard_stream == 0);
    get_component_context()->suspend_point = h;
  }

  constexpr void await_resume() const noexcept {}

};
