// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/futex.h>

#include "common/wrappers/copyable-atomic.h"

// This Mutex can is copyable, std::mutex is not
class CustomMutex {
 public:
  void Lock() {
    int old = kFree;
    if (state_.compare_exchange_strong(old, kLockedNoWaiters)) {
      return;
    }
    if (old != kLockedWithWaiters) {
      // was at least one waiter
      old = state_.exchange(kLockedWithWaiters);
    }
    while (old != kFree) {
      syscall(SYS_futex, &state_, FUTEX_WAIT, kLockedWithWaiters, 0, 0, 0);
      old = state_.exchange(kLockedWithWaiters);
    }
  }

  void Unlock() {
    if (state_.fetch_sub(1) == kLockedWithWaiters) {
      state_.store(kFree);
      syscall(SYS_futex, &state_, FUTEX_WAKE, 1, 0, 0, 0); // wake one
    }
  }

  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

 private:
  static constexpr int kFree = 0;
  static constexpr int kLockedNoWaiters = 1;
  static constexpr int kLockedWithWaiters = 2; // really "may be with waiters"
  vk::copyable_atomic_integral<int> state_ = kFree;
};


template<class T>
void lock(T locker) {
  locker->lock();
}

template<class T>
void unlock(T locker) {
  locker->unlock();
}

inline void lock(CustomMutex &m) {
  m.Lock();
}

inline void unlock(CustomMutex &m) {
  m.Unlock();
}

class Lockable {
private:
  CustomMutex m;
public:
  Lockable() = default;
  virtual ~Lockable() = default;

  void lock() {
    ::lock(m);
  }

  void unlock() {
    ::unlock(m);
  }
};

template<class DataT>
class AutoLocker {
private:
  DataT ptr;
public:
  explicit AutoLocker(DataT ptr) :
    ptr(ptr) {
    lock(ptr);
  }

  ~AutoLocker() {
    unlock(ptr);
  }
};
