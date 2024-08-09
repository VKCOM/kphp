// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <coroutine>
#include <cstdint>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/fork/fork.h"
#include "runtime-light/utils/concepts.h"

class WaitQueue {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

  unordered_map<int64_t, task_t<fork_result>> forks;
  unordered_map<int64_t, task_t<fork_result>::awaiter_t> fork_awaiters;
  deque<std::coroutine_handle<>> awaited_handles;

  void resume_awaited_handles_if_empty();

public:
  WaitQueue(const WaitQueue &) = delete;
  WaitQueue &operator=(const WaitQueue &) = delete;
  WaitQueue &operator=(WaitQueue &&) = delete;

  WaitQueue(WaitQueue &&other) noexcept
    : forks(std::move(other.forks))
    , fork_awaiters(std::move(other.fork_awaiters))
    , awaited_handles(std::move(other.awaited_handles)) {}

  explicit WaitQueue(memory_resource::unsynchronized_pool_resource &memory_resource, unordered_map<int64_t, task_t<fork_result>> &&forks_) noexcept
    : forks(std::move(forks_))
    , fork_awaiters(unordered_map<int64_t, task_t<fork_result>::awaiter_t>::allocator_type{memory_resource})
    , awaited_handles(deque<std::coroutine_handle<>>::allocator_type{memory_resource}) {
    for (auto &fork : forks) {
      fork_awaiters.emplace(fork.first, std::addressof(fork.second));
    }
  }

  void push_fork(int64_t fork_id, task_t<fork_result> &&fork) noexcept {
    auto [fork_it, fork_success] = forks.emplace(fork_id, std::move(fork));
    auto [awaiter_id, awaiter_success] = fork_awaiters.emplace(fork_id, std::addressof(fork_it->second));
    if (!awaited_handles.empty()) {
      awaiter_id->second.await_suspend(awaited_handles.front());
    }
  }

  Optional<std::pair<int64_t, task_t<fork_result>>> pop_fork() noexcept {
    auto it = std::find_if(forks.begin(), forks.end(), [](std::pair<const int64_t, task_t<fork_result>> &fork) { return fork.second.done(); });
    if (it != forks.end()) {
      auto fork = std::move(*it);
      forks.erase(fork.first);
      fork_awaiters.erase(fork.first);
      resume_awaited_handles_if_empty();
      return fork;
    } else {
      return {};
    }
  }

  void push_coro_handle(std::coroutine_handle<> coro) noexcept {
    if (awaited_handles.empty()) {
      std::for_each(fork_awaiters.begin(), fork_awaiters.end(), [coro](auto &awaiter) { awaiter.second.await_suspend(coro); });
    }
    awaited_handles.push_back(coro);
  }

  void pop_coro_handle() noexcept {
    if (!awaited_handles.empty()) {
      awaited_handles.pop_front();
    }
    std::coroutine_handle<> next_awaiter = awaited_handles.empty() ? std::noop_coroutine() : awaited_handles.front();
    std::for_each(fork_awaiters.begin(), fork_awaiters.end(), [&](auto &awaiter) { awaiter.second.await_suspend(next_awaiter); });
  }

  bool has_ready_fork() const noexcept {
    return std::any_of(forks.begin(), forks.end(), [](const auto &fork) { return fork.second.done(); });
  }

  size_t size() const noexcept {
    return forks.size();
  }

  bool empty() const noexcept {
    return forks.empty();
  }
};
