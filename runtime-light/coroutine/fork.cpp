#include "runtime-light/coroutine/fork.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/timer.h"

KphpForkContext &KphpForkContext::current() {
  return get_component_context()->kphp_fork_context;
}

void fork_scheduler::unregister_fork(int64_t fork_id) noexcept {
  php_debug("unregister fork %ld", fork_id);
  running_forks.erase(fork_id);
  ready_forks.erase(fork_id);
}

bool fork_scheduler::is_fork_ready(int64_t fork_id) noexcept {
  return ready_forks.contains(fork_id);
}

void fork_scheduler::mark_fork_ready_to_resume(int64_t fork_id) noexcept {
  if (is_fork_not_canceled(fork_id)) {
    forks_ready_to_resume.push_back(fork_id);
  }
}

bool fork_scheduler::is_fork_not_canceled(int64_t fork_id) noexcept {
  return running_forks.contains(fork_id);
}

bool fork_scheduler::is_main_fork_finish() noexcept {
  return running_forks[KphpForkContext::main_fork_id].task.done();
}

light_fork &fork_scheduler::get_fork_by_id(int64_t fork_id) noexcept {
  return running_forks[fork_id];
}

void fork_scheduler::resume_fork_by_future(runtime_future awaited_future) noexcept {
  if (!future_to_blocked_fork.contains(awaited_future)) {
    return;
  }
  int64_t blocked_fork = future_to_blocked_fork[awaited_future];
  // delete info of future waiting
  future_to_blocked_fork.erase(awaited_future);
  fork_to_awaited_future.erase(blocked_fork);

  // free associated timer
  std::visit([this](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    int64_t timer_d = 0;
    if constexpr (std::is_same_v<T, fork_future>) {
      timer_d = arg.timeout_d;
    } else if constexpr (std::is_same_v<T, stream_future>) {
      timer_d = arg.timeout_d;
    } else {}
    if (timer_d != 0) {
      timer_to_blocked_fork.erase(timer_d);
      free_descriptor(timer_d);
    }
  }, awaited_future);

  mark_fork_ready_to_resume(blocked_fork);
}

void fork_scheduler::resume_fork(int64_t fork_id) noexcept {
  php_assert(yielded_forks.contains(fork_id));
  php_debug("resume fork that was yield %ld", fork_id);
  mark_fork_ready_to_resume(fork_id);
}

void fork_scheduler::resume_fork_by_incoming_query() noexcept {
  if (!wait_incoming_query_forks.empty()) {
    php_debug("resume fork to process incoming query");
    int fork_id = *wait_incoming_query_forks.begin();
    wait_incoming_query_forks.erase(fork_id);
    mark_fork_ready_to_resume(fork_id);
  }
}

void fork_scheduler::resume_fork_by_timeout(int64_t timer_d) noexcept {
  if (timer_to_blocked_fork.contains(timer_d)) {
    int64_t blocked_fork = timer_to_blocked_fork[timer_d];
    php_debug("resume fork %ld by timeout %ld", blocked_fork, timer_d);
    runtime_future awaited_future = fork_to_awaited_future.at(blocked_fork);
    // delete info of future waiting
    future_to_blocked_fork.erase(awaited_future);
    fork_to_awaited_future.erase(blocked_fork);
    timer_to_blocked_fork.erase(timer_d);

    mark_fork_ready_to_resume(blocked_fork);
  }
}

void fork_scheduler::mark_current_fork_as_ready() noexcept {
  int64_t fork_id = KphpForkContext::current().current_fork_id;
  php_debug("fork %lu is ready", fork_id);
  ready_forks.insert(fork_id);
}

void fork_scheduler::block_fork_on_future(int64_t blocked_fork, std::coroutine_handle<> handle, runtime_future awaited_future, double timeout) {
  php_debug("block fork %ld on future type %zu", blocked_fork, awaited_future.index());
  if (future_to_blocked_fork.contains(awaited_future)) {
    php_warning("future is already waited");
    return;
  }
  uint64_t timer_d = 0;
  if (timeout >= 0) {
    timer_d = set_timer_without_callback(timeout);
    if (timer_d > 0) {
      timer_to_blocked_fork[timer_d] = blocked_fork;
    } else {
      php_warning("cannot set up timer for another fork waiting");
    }
  }
  future_to_blocked_fork[awaited_future] = blocked_fork;
  fork_to_awaited_future[blocked_fork] = awaited_future;
  running_forks[blocked_fork].handle = handle;
}

void fork_scheduler::yield_fork(int64_t fork_id, std::coroutine_handle<> handle) noexcept {
  php_debug("yield fork %ld", fork_id);
  yielded_forks.insert(fork_id);
  running_forks[fork_id].handle = handle;
}

void fork_scheduler::block_fork_on_incoming_query(int64_t fork_id, std::coroutine_handle<> handle) noexcept {
  php_assert(wait_incoming_query_forks.empty());
  php_debug("block fork on incoming query fork_id %ld", fork_id);
  wait_incoming_query_forks.insert(fork_id);
  running_forks[fork_id].handle = handle;
}

int64_t fork_scheduler::start_fork(task_t<fork_result> &&task) noexcept {
  int64_t fork_id = ++KphpForkContext::current().last_registered_fork_id;
  int64_t parent_fork_id = KphpForkContext::current().current_fork_id;
  php_debug("start fork %ld from fork %ld", fork_id, parent_fork_id);
  KphpForkContext::current().current_fork_id = fork_id;
  running_forks[fork_id] = light_fork(std::move(task));
  running_forks[fork_id].handle();
  KphpForkContext::current().current_fork_id = parent_fork_id;
  return fork_id;
}

void fork_scheduler::register_main_fork(light_fork &&fork) noexcept {
  running_forks[KphpForkContext::main_fork_id] = std::move(fork);
  mark_fork_ready_to_resume(KphpForkContext::main_fork_id);
}

void fork_scheduler::schedule() noexcept {
  php_debug("start fork schedule");
  while (true) {
    php_debug("schedule [yielded forks %zu, ready_forks %zu, forks_ready_to_resume %zu, running forks %zu]", yielded_forks.size(), ready_forks.size(),
              forks_ready_to_resume.size(), running_forks.size());
    php_debug("awaiting forks [wait_incoming_query_forks %zu, future_to_blocked_fork %zu]", wait_incoming_query_forks.size(),
              future_to_blocked_fork.size());
    // Resume all yielded forks
    for (int64_t fork_id : yielded_forks) {
      resume_fork(fork_id);
    }
    yielded_forks.clear();

    // Resume blocked forks that wait for another fork
    for (int64_t fork_id : ready_forks) {
      resume_fork_by_future(fork_future{fork_id});
    }

    if (forks_ready_to_resume.empty()) {
      // No ready forks to resume. That means that component is blocked
      php_debug("all forks blocked. Set blocked status [wait_incoming_query_forks %zu, future_to_blocked_fork %zu, running_forks %zu]",
                wait_incoming_query_forks.size(), future_to_blocked_fork.size(), running_forks.size());
      componentState->poll_status = PollStatus::PollBlocked;
      return;
    }

    php_debug("resume forks [forks_ready_to_resume %zu, running forks %zu]", forks_ready_to_resume.size(), running_forks.size());
    while (!forks_ready_to_resume.empty()) {
      if (get_platform_context()->please_yield.load()) {
        // Stop running if platform ask
        php_debug("stop forks schedule because of platform ask");
        componentState->poll_status = PollStatus::PollReschedule;
        return;
      }
      int64_t fork_id = forks_ready_to_resume.front();
      forks_ready_to_resume.pop_front();
      if (!is_fork_not_canceled(fork_id)) {
        continue;
      }
      scheduler_iteration(fork_id);
      if (is_main_fork_finish()) {
        break;
      }
    }

    if (is_main_fork_finish()) {
      php_debug("finish main fork");
      php_assert(running_forks.size() == 1);
      componentState->poll_status = PollStatus::PollFinishedOk;
      return;
    }
  }
}

void fork_scheduler::scheduler_iteration(int64_t fork_id) noexcept {
  php_debug("scheduler iteration on fork %ld", fork_id);
  if (!is_fork_not_canceled(fork_id)) {
    // fork may be canceled here by parent
    return;
  }
  auto &task = running_forks[fork_id];
  // Set global current fork id
  int64_t parent_fork_id = KphpForkContext::current().current_fork_id;
  KphpForkContext::current().current_fork_id = fork_id;
  task.handle();
  KphpForkContext::current().current_fork_id = parent_fork_id;
  if (task.task.done()) {
    php_debug("fork %ld is done", fork_id);
    ready_forks.insert(fork_id);
  }
}
