// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/thread-pool.h"

void ThreadPool::init() noexcept {
  int thread_pool_size = static_cast<int>(std::thread::hardware_concurrency() * thread_pool_ratio);
  if (!is_thread_pool_available() && thread_pool_size > 0) {
    thread_pool_ptr = new BS::thread_pool(thread_pool_size);
  }
}

double thread_pool_ratio = 0;
