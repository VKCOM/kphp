// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cmath>

#include "runtime/thread-pool.h"
#include "server/php-engine-vars.h"


void ThreadPool::init() noexcept {
  if (!is_thread_pool_available() && thread_pool_size > 0) {
    /**
     * linux does not determinize the thread to which the signal will arrive,
     * for this purpose runtime signals is blocked at thread creation time. Since they inherit the signaling mask
     * */
    sigset_t sigset = dl_get_empty_sigset();
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGUSR2);
    sigaddset(&sigset, SIGPHPASSERT);
    sigaddset(&sigset, SIGSTACKOVERFLOW);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    thread_pool_ptr = new BS::thread_pool(thread_pool_size);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
  }
}

uint64_t thread_pool_size = 0;
