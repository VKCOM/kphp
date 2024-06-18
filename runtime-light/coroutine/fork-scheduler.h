// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>
#include <cstdint>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-light/coroutine/light-fork.h"
#include "runtime-light/coroutine/runtime-future.h"
#include "runtime-light/coroutine/task.h"

struct fork_scheduler {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<class T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;
  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  fork_scheduler(memory_resource::unsynchronized_pool_resource &memory_pool)
    : running_forks(unordered_map<int64_t, light_fork>::allocator_type{memory_pool})
    , ready_forks(unordered_set<int64_t>::allocator_type{memory_pool})
    , future_to_blocked_fork(unordered_map<runtime_future, int64_t>::allocator_type{memory_pool})
    , fork_to_awaited_future(unordered_map<int64_t, runtime_future>::allocator_type{memory_pool})
    , timer_to_blocked_fork(unordered_map<uint64_t, int64_t>::allocator_type{memory_pool})
    , wait_incoming_query_forks(unordered_set<int64_t>::allocator_type{memory_pool})
    , forks_ready_to_resume(deque<int64_t>::allocator_type{memory_pool}) {}

  void register_main_fork(light_fork &&fork) noexcept;
  void unregister_fork(int64_t fork_id) noexcept;
  bool is_fork_ready(int64_t fork_id) noexcept;
  light_fork &get_fork_by_id(int64_t fork_id) noexcept;

  /**
   * Functions for blocking can be called only in await_suspend in awaitables
   * */
  void block_fork_on_future(int64_t blocked_fork, std::coroutine_handle<> handle, runtime_future awaited_future, double timeout);
  void yield_fork(int64_t fork_id, std::coroutine_handle<> handle, int64_t timeout_ns) noexcept;
  void block_fork_on_incoming_query(int64_t fork_id, std::coroutine_handle<> handle) noexcept;

  /**
   * Functions to mark fork as ready. Forks resumes in schedule()
   * */
  void mark_fork_ready_by_future(runtime_future awaited_future) noexcept;
  void mark_fork_ready_by_timeout(int64_t timer_d) noexcept;
  void mark_fork_ready_by_incoming_query() noexcept;

  void schedule() noexcept;
  /*functions for using in codegen */
  int64_t start_fork(task_t<fork_result> &&task) noexcept;
  void mark_current_fork_as_ready() noexcept;

private:
  void mark_fork_ready_to_resume(int64_t fork_id) noexcept;
  bool is_fork_not_canceled(int64_t fork_id) noexcept;
  bool is_main_fork_finish() noexcept;
  void set_up_timeout_for_waiting(int64_t fork_id, int64_t timeout_ns) noexcept;
  void scheduler_iteration(int64_t fork_id) noexcept;
  void remove_info_about_fork_wait_future(int64_t fork_id, runtime_future awaited_future) noexcept;

  /* map of running forks. fork_id to task_t structure */
  unordered_map<int64_t, light_fork> running_forks;
  /* set of ready forks */
  unordered_set<int64_t> ready_forks;
  /* map from runtime_future {stream_future, fork_future} to waiting fork*/
  unordered_map<runtime_future, int64_t> future_to_blocked_fork;
  /* reverse of future_to_blocked_fork */
  unordered_map<int64_t, runtime_future> fork_to_awaited_future;
  /* timeout timer_d to waiting fork */
  unordered_map<uint64_t, int64_t> timer_to_blocked_fork;
  /* set of forks that wait to accept incoming stream */
  unordered_set<int64_t> wait_incoming_query_forks;

  deque<int64_t> forks_ready_to_resume;
};
