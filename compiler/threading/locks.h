// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <unistd.h>
#include "compiler/kphp_assert.h"

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

inline bool try_lock(volatile int *locker) {
  return __sync_lock_test_and_set(locker, 1) == 0;
}

inline void lock(volatile int *locker) {
  while (!try_lock(locker)) {
    usleep(250);
  }
}

inline void unlock(volatile int *locker) {
  if (*locker != 1) {
    std::string s = "Real value when unlock = " + std::to_string(*locker);
    fprintf(stdout, "%s\n", s.c_str());
    fflush(stdout);
    abort();
  }
  __sync_lock_release(locker);
}

class Lockable {
private:
  volatile int x;
public:
  Lockable() :
    x(0) {}

  virtual ~Lockable() = default;

  void lock() {
    ::lock(&x);
  }

  bool try_lock() {
    return ::try_lock(&x);
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
