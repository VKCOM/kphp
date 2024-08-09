// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/wait-queue.h"

#include "runtime-light/component/component.h"

void WaitQueue::resume_awaited_handles_if_empty() {
  if (forks.empty()) {
    while (!awaited_handles.empty()) {
      auto handle = awaited_handles.front();
      awaited_handles.pop_front();
      CoroutineScheduler::get().suspend({handle, WaitEvent::Rechedule{}});
    }
  }
}

