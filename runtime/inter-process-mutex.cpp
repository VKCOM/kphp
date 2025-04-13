// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/inter-process-mutex.h"

#include <cerrno>
#include <cstring>
#include <sys/syscall.h>

#include "common/macos-ports.h"

#include "runtime/critical_section.h"
#include "runtime/php_assert.h"
#include "server/php-engine-vars.h"

pid_t get_main_thread_id() noexcept {
  return pid;
}

void __attribute__((noinline)) check_that_tid_and_cached_pid_same() noexcept {
  // to avoid calling gettid syscall each time, trust pid global var, but check periodically
  // if this assert trigger, that means that runtime use several threads or pid is not updated
  php_assert(get_main_thread_id() == syscall(SYS_gettid));
}

long __attribute__((noinline)) futex(pid_t* lock, int command) noexcept {
#if defined(__APPLE__)
  static_cast<void>(lock);
  static_cast<void>(command);
  errno = EAGAIN;
  return -1;
#else
  return syscall(SYS_futex, lock, command, 0, nullptr, nullptr, 0);
#endif
}

void handle_lock_error(pid_t* lock, const char* what) noexcept {
  if (errno == ESRCH) {
    if (const pid_t dead_owner = __sync_fetch_and_add(lock, 0)) {
      if (dead_owner & FUTEX_WAITERS) {
        __sync_bool_compare_and_swap(lock, dead_owner, 0);
      }
    }
  } else if (errno != EAGAIN) {
    php_runtime_critical("Got unexpected error from futex %s: %s", what, std::strerror(errno));
  }
}

void inter_process_mutex::lock() noexcept {
  dl::enter_critical_section();
  const pid_t tid = get_main_thread_id();
  constexpr size_t trylock_period = 512;
  constexpr size_t lock_period = trylock_period * 4;
  size_t counter = 0;
  bool tid_checked = false;
  while (!__sync_bool_compare_and_swap(&lock_, 0, tid)) {
    ++counter;
    if (counter % lock_period == 0) {
      if (!tid_checked) {
        check_that_tid_and_cached_pid_same();
        tid_checked = true;
      }
      if (!futex(&lock_, FUTEX_LOCK_PI)) {
        return;
      } else {
        handle_lock_error(&lock_, "lock");
      }
    } else if (counter % trylock_period == 0) {
      if (!futex(&lock_, FUTEX_TRYLOCK_PI)) {
        return;
      } else {
        handle_lock_error(&lock_, "try lock");
      }
    }
    __sync_synchronize();
  }
}

bool inter_process_mutex::try_lock() noexcept {
  dl::enter_critical_section();
  const pid_t tid = get_main_thread_id();
  // try to lock two times as the first attempt can fail if the previous owner is dead
  for (size_t attempts = 0; attempts != 2; ++attempts) {
    if (__sync_bool_compare_and_swap(&lock_, 0, tid) || !futex(&lock_, FUTEX_TRYLOCK_PI)) {
      return true;
    }
    handle_lock_error(&lock_, "try lock");
  }
  dl::leave_critical_section();
  return false;
}

void inter_process_mutex::unlock() noexcept {
  const pid_t tid = get_main_thread_id();
  if (!__sync_bool_compare_and_swap(&lock_, tid, 0)) {
    if (futex(&lock_, FUTEX_UNLOCK_PI)) {
      php_runtime_critical("Got unexpected error from futex unlock: %s", std::strerror(errno));
    }
  }
  dl::leave_critical_section();
}
