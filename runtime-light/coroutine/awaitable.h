#pragma once

#include <coroutine>

#include "runtime-light/component/component.h"
#include "runtime-light/utils/logs.h"

struct blocked_operation_t {
  uint64_t awaited_stream;

  blocked_operation_t(uint64_t stream_d) : awaited_stream(stream_d) {}

  constexpr bool await_ready() const noexcept {
    return false;
  }

  void await_resume() const noexcept {
    ComponentState & ctx = *get_component_context();
    ctx.opened_streams[awaited_stream] = NotBlocked;
  }
};

struct read_blocked_t : blocked_operation_t {
  void await_suspend(std::coroutine_handle<> h) const noexcept {
    php_debug("blocked read on stream %lu", awaited_stream);
    ComponentState & ctx = *get_component_context();
    ctx.poll_status = PollStatus::PollBlocked;
    ctx.opened_streams[awaited_stream] = RBlocked;
    ctx.awaiting_coroutines[awaited_stream] = h;
  }
};

struct write_blocked_t : blocked_operation_t {
  void await_suspend(std::coroutine_handle<> h) const noexcept {
    php_debug("blocked write on stream %lu", awaited_stream);
    ComponentState & ctx = *get_component_context();
    ctx.poll_status = PollStatus::PollBlocked;
    ctx.opened_streams[awaited_stream] = WBlocked;
    ctx.awaiting_coroutines[awaited_stream] = h;
  }
};

struct test_yield_t {
  bool await_ready() const noexcept {
    return !get_platform_context()->please_yield.load();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    ComponentState & ctx = *get_component_context();
    ctx.poll_status = PollStatus::PollReschedule;
    ctx.standard_handle = h;
  }

  constexpr void await_resume() const noexcept {}
};

struct wait_input_query_t {
  bool await_ready() const noexcept {
    return !get_component_context()->incoming_pending_queries.empty();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    ComponentState & ctx = *get_component_context();
    php_assert(ctx.standard_stream == 0);
    ctx.standard_handle = h;
  }

  constexpr void await_resume() const noexcept {}
};
