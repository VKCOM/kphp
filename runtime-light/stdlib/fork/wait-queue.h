// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <coroutine>

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/utils/concepts.h"

class wait_queue_t {
  template<hashable T>
  using unordered_set = memory_resource::stl::unordered_set<T, memory_resource::unsynchronized_pool_resource>;

  template<typename T>
  using deque = memory_resource::stl::deque<T, memory_resource::unsynchronized_pool_resource>;

public:
  struct awaiter_t {
    wait_queue_t *wait_queue;
    std::coroutine_handle<> awaited_handle;
    explicit awaiter_t(wait_queue_t *wait_queue_) noexcept
      : wait_queue(wait_queue_)
      , awaited_handle(std::noop_coroutine()) {}

    bool await_ready() const noexcept {
      return wait_queue->has_ready_fork();
    }

    void await_suspend(std::coroutine_handle<> coro) noexcept {
      awaited_handle = coro;
      wait_queue->insert_awaiter(*this);
    }

    int64_t await_resume() noexcept {
      wait_queue->erase_awaiter(*this);
      return wait_queue->pop_ready_fork();
    }

    bool resumable() noexcept {
      return wait_queue->has_ready_fork();
    }

    void cancel() noexcept {
      wait_queue->erase_awaiter(*this);
    }

    bool operator==(const awaiter_t &other) const {
      return awaited_handle == other.awaited_handle;
    }
  };

  wait_queue_t(const wait_queue_t &) = delete;
  wait_queue_t &operator=(const wait_queue_t &) = delete;
  wait_queue_t &operator=(wait_queue_t &&) = delete;

  wait_queue_t(wait_queue_t &&other) noexcept
    : forks_ids(std::move(other.forks_ids))
    , awaiters(std::move(other.awaiters)) {}

  explicit wait_queue_t(memory_resource::unsynchronized_pool_resource &memory_resource, unordered_set<int64_t> &&forks_ids_) noexcept;

  void push(int64_t fork_id) noexcept;

  awaiter_t pop() noexcept {
    return awaiter_t{this};
  }

  bool empty() const noexcept {
    return forks_ids.empty();
  }

private:
  unordered_set<int64_t> forks_ids;
  deque<awaiter_t> awaiters;

  void insert_awaiter(const awaiter_t &awaiter) noexcept;
  void erase_awaiter(const awaiter_t &awaiter) noexcept;
  int64_t pop_ready_fork() noexcept;
  bool has_ready_fork() const noexcept;
};
