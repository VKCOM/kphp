// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/wait-queue.h"

#include "runtime-light/stdlib/fork/fork-context.h"

wait_queue_t::wait_queue_t(memory_resource::unsynchronized_pool_resource &memory_resource, unordered_set<int64_t> &&forks_ids_) noexcept
  : forks_ids(std::move(forks_ids_))
  , awaiters(deque<awaiter_t>::allocator_type{memory_resource}) {
  std::for_each(forks_ids.begin(), forks_ids.end(), [](int64_t fork_id) { ForkComponentContext::get().reserve_fork(fork_id); });
}

void wait_queue_t::push(int64_t fork_id) noexcept {
  forks_ids.insert(fork_id);
  ForkComponentContext::get().reserve_fork(fork_id);
  if (!awaiters.empty()) {
    auto &task = ForkComponentContext::get().forks[fork_id].first;
    task_t<fork_result>::awaiter_t awaiter{std::addressof(task)};
    awaiter.await_suspend(awaiters.front().awaited_handle);
  }
}

void wait_queue_t::insert_awaiter(const awaiter_t &awaiter) noexcept {
  if (awaiters.empty()) {
    std::for_each(forks_ids.begin(), forks_ids.end(), [&](int64_t fork_id) {
      task_t<fork_result>::awaiter_t task_awaiter{std::addressof(ForkComponentContext::get().forks[fork_id].first)};
      task_awaiter.await_suspend(awaiter.awaited_handle);
    });
  }
  awaiters.push_back(awaiter);
}

void wait_queue_t::erase_awaiter(const awaiter_t &awaiter) noexcept {
  auto it = std::find(awaiters.begin(), awaiters.end(), awaiter);
  if (it != awaiters.end()) {
    awaiters.erase(it);
  }
  std::coroutine_handle<> next_awaiter = awaiters.empty() ? std::noop_coroutine() : awaiters.front().awaited_handle;
  std::for_each(forks_ids.begin(), forks_ids.end(), [&](int64_t fork_id) {
    task_t<fork_result>::awaiter_t task_awaiter{std::addressof(ForkComponentContext::get().forks[fork_id].first)};
    task_awaiter.await_suspend(next_awaiter);
  });
}

int64_t wait_queue_t::pop_ready_fork() noexcept {
  auto it = std::find_if(forks_ids.begin(), forks_ids.end(), [](int64_t fork_id) { return ForkComponentContext::get().is_ready(fork_id); });
  if (it == forks_ids.end()) {
    php_critical_error("there is no fork to pop from queue");
  }
  int64_t ready_fork = *it;
  forks_ids.erase(it);
  ForkComponentContext::get().unreserve_fork(ready_fork);
  return ready_fork;
}

bool wait_queue_t::has_ready_fork() const noexcept {
  return std::any_of(forks_ids.begin(), forks_ids.end(), [](int64_t fork_id) { return ForkComponentContext::get().is_ready(fork_id); });
}
