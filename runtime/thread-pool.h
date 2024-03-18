// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/dl-utils-lite.h"
#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "third-party/BS_thread_pool.hpp"

class ThreadPool : vk::not_copyable {
public:
  void init() noexcept;

  bool is_thread_pool_available() const noexcept {
    return thread_pool_ptr != nullptr;
  }

  BS::thread_pool &pool() noexcept {
    dl_assert(is_thread_pool_available(), "thread pool in uninitialized or initialized with size less or equal to zero") return *thread_pool_ptr;
  }

  void stop() noexcept {
    /** Here we need to stop all running threads to ensure independence of requests
     * todo find a way to kill threads. It can be done by pthread_kill
     * */
    if (thread_pool_ptr != nullptr) {
      thread_pool_ptr->wait_for_tasks();
    }
  }

private:
  ThreadPool() = default;

  friend class vk::singleton<ThreadPool>;

  BS::thread_pool *thread_pool_ptr{nullptr};
};

extern uint32_t thread_pool_size;
