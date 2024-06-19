#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/core/small-object-storage.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/streams.h"

// Type erasure of stored object
struct fork_result {
  small_object_storage<sizeof(mixed)> storage;

  template<typename T>
  fork_result(T &&t) {
    storage.emplace<std::remove_cvref_t<T>>(std::forward<T>(t));
  }

  template<typename T>
  T get() {
    return *storage.get<T>();
  }
};

// Universal task type for any type of fork
struct light_fork {
  task_t<fork_result> task;
  std::coroutine_handle<> handle;

  light_fork() = default;

  light_fork(task_t<fork_result> &&other)
    : task(std::move(other)) {
    handle = task.get_handle();
  }

  template<typename T>
  T get_fork_result() {
    return task.get_result().get<T>();
  }
};

struct fork_scheduler {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  template<class T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;


  fork_scheduler(memory_resource::unsynchronized_pool_resource &memory_pool)
    : running_forks(unordered_map<int64_t, light_fork>::allocator_type{memory_pool})
    , forks_ready_to_resume(unordered_set<int64_t>::allocator_type{memory_pool})
    , ready_forks(unordered_set<int64_t>::allocator_type{memory_pool})
    , yielded_forks(unordered_set<int64_t>::allocator_type{memory_pool})
    , wait_incoming_query_forks(unordered_set<int64_t>::allocator_type{memory_pool})
    , wait_stream_forks(unordered_map<int64_t, std::pair<int64_t, StreamRuntimeStatus>>::allocator_type{memory_pool})
    , wait_another_fork_forks(unordered_map<int64_t, int64_t>::allocator_type{memory_pool}) {}

  void register_main_fork(light_fork && fork) noexcept;
  void unregister_fork(int64_t fork_id) noexcept;
  bool is_fork_ready(int64_t fork_id) noexcept;
  light_fork &get_fork_by_id(int64_t fork_id) noexcept;

  void block_fork_on_another_fork(int64_t blocked_fork, std::coroutine_handle<> handle, int64_t expected_fork) noexcept;
  void block_fork_on_stream(int64_t blocked_fork, std::coroutine_handle<> handle, uint64_t stream_d, StreamRuntimeStatus status) noexcept;
  void yield_fork(int64_t fork_id, std::coroutine_handle<> handle) noexcept;
  void block_fork_on_incoming_query(int64_t fork_id, std::coroutine_handle<> handle) noexcept;

  void resume_fork_by_another_fork(int64_t expected_fork) noexcept;
  void resume_fork_by_stream(uint64_t stream_d, const StreamStatus &status) noexcept;
  void resume_fork(int64_t fork_id) noexcept;
  void resume_fork_by_incoming_query() noexcept;

  void schedule() noexcept;
  /*functions for using in codegen */
  int64_t start_fork(task_t<fork_result> && task) noexcept;
  void mark_current_fork_as_ready() noexcept;

private:
  bool is_main_fork_finish() noexcept;
  void scheduler_iteration(int64_t fork_id) noexcept;

  unordered_map<int64_t, light_fork> running_forks;
  unordered_set<int64_t> forks_ready_to_resume;
  unordered_set<int64_t> ready_forks;

  unordered_set<int64_t> yielded_forks;
  unordered_set<int64_t> wait_incoming_query_forks;
  unordered_map<int64_t, std::pair<int64_t, StreamRuntimeStatus>> wait_stream_forks;
  unordered_map<int64_t, int64_t> wait_another_fork_forks;
};

struct KphpForkContext {

  static constexpr int64_t main_fork_id = 0;

  static KphpForkContext &current();

  KphpForkContext(memory_resource::unsynchronized_pool_resource &memory_pool)
    : scheduler(memory_pool) {}

  int64_t current_fork_id = main_fork_id;
  int last_registered_fork_id = main_fork_id;

  fork_scheduler scheduler;
};
