#include "runtime-light/coroutine/fork.h"
#include "runtime-light/component/component.h"

KphpForkContext &KphpForkContext::current() {
  return get_component_context()->kphp_fork_context;
}

void fork_scheduler::unregister_fork(int64_t fork_id) noexcept {
  php_debug("unregister fork %ld", fork_id);
  running_forks.erase(fork_id);
  forks_ready_to_resume.erase(fork_id);
  ready_forks.erase(fork_id);
}

bool fork_scheduler::is_fork_ready(int64_t fork_id) noexcept {
  return ready_forks.contains(fork_id);
}

bool fork_scheduler::is_main_fork_finish() noexcept {
  return running_forks[KphpForkContext::main_fork_id].task.done();
}

light_fork &fork_scheduler::get_fork_by_id(int64_t fork_id) noexcept {
  return running_forks[fork_id];
}

void fork_scheduler::resume_fork_by_another_fork(int64_t expected_fork) noexcept {
  php_assert(ready_forks.contains(expected_fork));
  if (wait_another_fork_forks.contains(expected_fork)) {
    forks_ready_to_resume.insert(wait_another_fork_forks[expected_fork]);
    wait_another_fork_forks.erase(expected_fork);
  }
}

void fork_scheduler::resume_fork_by_stream(uint64_t stream_d, const StreamStatus &status) noexcept {
  if (wait_stream_forks.contains(stream_d)) {
    auto [fork_id, blocked_status] = wait_stream_forks[stream_d];
    if ((blocked_status == WBlocked && status.write_status != IOBlocked) ||
        (blocked_status == RBlocked && status.read_status != IOBlocked)) {
      php_debug("update fork %ld status on stream %lu", fork_id, stream_d);
      wait_stream_forks.erase(stream_d);
      forks_ready_to_resume.insert(fork_id);
    }
  }
}

void fork_scheduler::resume_fork(int64_t fork_id) noexcept {
  php_assert(yielded_forks.contains(fork_id));
  forks_ready_to_resume.insert(fork_id);
  yielded_forks.erase(fork_id);
}

void fork_scheduler::resume_fork_by_incoming_query() noexcept {
  if (!wait_incoming_query_forks.empty()) {
    php_debug("resume fork to process incoming query");
    int fork_id = *wait_incoming_query_forks.begin();
    forks_ready_to_resume.insert(fork_id);
    wait_incoming_query_forks.erase(fork_id);
  }
}

void fork_scheduler::mark_current_fork_as_ready() noexcept {
  int64_t fork_id = KphpForkContext::current().current_fork_id;
  php_debug("fork %lu is ready", fork_id);
  ready_forks.insert(fork_id);
}

void fork_scheduler::block_fork_on_another_fork(int64_t blocked_fork, std::coroutine_handle<> handle, int64_t expected_fork) noexcept {
  php_debug("block fork %ld on another fork %ld", blocked_fork, expected_fork);
  wait_another_fork_forks[expected_fork] = blocked_fork;
  running_forks[blocked_fork].handle = handle;
}

void fork_scheduler::block_fork_on_stream(int64_t blocked_fork, std::coroutine_handle<> handle, uint64_t stream_d, StreamRuntimeStatus status) noexcept {
  php_debug("block fork %ld on stream %lu", blocked_fork, stream_d);
  wait_stream_forks[stream_d] = {blocked_fork, status};
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

int64_t fork_scheduler::start_fork(task_t<fork_result> && task) noexcept {
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
  forks_ready_to_resume.insert(KphpForkContext::main_fork_id);
}

void fork_scheduler::schedule() noexcept {
  php_debug("start fork schedule");
  while (true) {
    // Resume all yielded forks
    for (int64_t fork_id : yielded_forks) {
      resume_fork(fork_id);
    }

    // Resume blocked forks that wait for another fork
    for (int64_t fork_id : ready_forks) {
      resume_fork_by_another_fork(fork_id);
    }

    if (forks_ready_to_resume.empty()) {
      // No ready forks to resume. That means that component is blocked
      php_debug("all forks blocked. Set blocked status");
      componentState->poll_status = PollStatus::PollBlocked;
      return;
    }

    for (auto it = forks_ready_to_resume.cbegin(); it != forks_ready_to_resume.cend();) {
      if (get_platform_context()->please_yield.load()) {
        // Stop running if platform ask
        php_debug("stop forks schedule because of yield");
        componentState->poll_status = PollStatus::PollReschedule;
        return;
      }
      scheduler_iteration(*it);
      php_assert(forks_ready_to_resume.contains(*it));
      it = forks_ready_to_resume.erase(it);
    }

    if (is_main_fork_finish()) {
      php_debug("finish main fork");
      componentState->poll_status = PollStatus::PollFinishedOk;
      return;
    }
  }
}

void fork_scheduler::scheduler_iteration(int64_t fork_id) noexcept {
  php_debug("scheduler iteration on fork %ld", fork_id);
  auto & task = running_forks[fork_id];
  // Set global current fork id
  int64_t parent_fork_id = KphpForkContext::current().current_fork_id;
  KphpForkContext::current().current_fork_id = fork_id;
  task.handle();
  KphpForkContext::current().current_fork_id = parent_fork_id;
  if (task.task.done()) {
    php_debug("fork %ld is done", fork_id);
    ready_forks.insert(fork_id);
  }
  php_debug("end of scheduler iteration");
}
