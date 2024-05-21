// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cassert>
#include <unistd.h>

#include "common/cacheline.h"

enum { LOCKED = 1, UNLOCKED = 0 };

template<class T>
bool try_lock(T);

template<class T>
void lock(T locker) {
  locker->lock();
}

template<class T>
void unlock(T locker) {
  locker->unlock();
}

inline bool try_lock(std::atomic<int> *locker) {
  int expected = 0;
  return locker->compare_exchange_weak(expected, LOCKED, std::memory_order_acq_rel);
}

inline void lock(std::atomic<int> *locker) {
  while (!try_lock(locker)) {
    usleep(250);
  }
}

inline void unlock(std::atomic<int> *locker) {
  assert(locker->load(std::memory_order_relaxed) == LOCKED);
  locker->store(UNLOCKED, std::memory_order_release);
}

class KDB_CACHELINE_ALIGNED Lockable {
private:
  std::atomic<int> x;
public:
  Lockable() :
    x(0) {}

  virtual ~Lockable() = default;

  void lock() {
    ::lock(&x);
  }

  void unlock() {
    ::unlock(&x);
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
