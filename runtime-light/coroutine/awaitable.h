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

  void await_resume() const noexcept {}
};

struct read_blocked_t : blocked_operation_t {
  void await_suspend(std::coroutine_handle<> h) const noexcept {
    php_debug("blocked read on stream %lu", awaited_stream);
    KphpForkContext & context = KphpForkContext::current();
    context.scheduler.block_fork_on_stream(context.current_fork_id, h, awaited_stream, RBlocked);
  }
};

struct write_blocked_t : blocked_operation_t {
  void await_suspend(std::coroutine_handle<> h) const noexcept {
    php_debug("blocked write on stream %lu", awaited_stream);
    KphpForkContext & context = KphpForkContext::current();
    context.scheduler.block_fork_on_stream(context.current_fork_id, h, awaited_stream, WBlocked);
  }
};

struct test_yield_t {
  bool await_ready() const noexcept {
    return !get_platform_context()->please_yield.load();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    KphpForkContext & context = KphpForkContext::current();
    php_debug("platform yield fork %ld", context.current_fork_id);
    context.scheduler.yield_fork(context.current_fork_id, h);
  }

  constexpr void await_resume() const noexcept {}
};

struct wait_incoming_query_t {
  bool await_ready() const noexcept {
    return !get_component_context()->incoming_pending_queries.empty();
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    ComponentState & ctx = *get_component_context();
    php_assert(ctx.standard_stream == 0);
    php_debug("fork %ld blocked on incoming query", ctx.kphp_fork_context.current_fork_id);
    ctx.wait_incoming_stream = true;
    ctx.kphp_fork_context.scheduler.block_fork_on_incoming_query(ctx.kphp_fork_context.current_fork_id, h);
  }

  void await_resume() const noexcept {
    get_component_context()->wait_incoming_stream = false;
  }
};

template<typename T>
struct wait_fork_t {
  int64_t expected_fork_id;

  wait_fork_t(int64_t id) : expected_fork_id(id) {}

  bool await_ready() const noexcept {
    return KphpForkContext::current().scheduler.is_fork_ready(expected_fork_id);
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    KphpForkContext & context = KphpForkContext::current();
    php_debug("fork %ld wait for fork %ld", context.current_fork_id, expected_fork_id);
    context.scheduler.block_fork_on_another_fork(context.current_fork_id, h, expected_fork_id);
  }

  T await_resume() const noexcept {
    auto & fork = KphpForkContext::current().scheduler.get_fork_by_id(expected_fork_id);
    php_assert(fork.task.done());
    const vk::final_action final_action([this]{ KphpForkContext::current().scheduler.unregister_fork(expected_fork_id);});
    return fork.get_fork_result<T>();
  }
};

struct sched_yield_t {
  bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const noexcept {
    KphpForkContext & context = KphpForkContext::current();
    php_debug("fork %ld sched yield", context.current_fork_id);
    context.scheduler.yield_fork(context.current_fork_id, h);
  }

  void await_resume() const noexcept {}
};
