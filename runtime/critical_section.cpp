// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/critical_section.h"

#include <atomic>
#include <signal.h>

#include "runtime/php_assert.h"

void check_stack_overflow() __attribute__ ((weak));

void check_stack_overflow() {
  //pass;
}

namespace dl {

std::atomic<int> in_critical_section;
volatile long long pending_signals;

void enter_critical_section() noexcept {
  check_stack_overflow();
  php_assert (in_critical_section.load(std::memory_order_relaxed) >= 0);
  in_critical_section.fetch_add(1, std::memory_order_release);
}

void leave_critical_section() noexcept {
  in_critical_section.fetch_sub(1, std::memory_order_release);
  php_assert (in_critical_section.load(std::memory_order_relaxed) >= 0);
  if (pending_signals && in_critical_section.load(std::memory_order_acquire) <= 0) {
    for (int i = 0; i < sizeof(pending_signals) * 8; i++) {
      if ((pending_signals >> i) & 1) {
        raise(i);
      }
    }
  }
}

void init_critical_section() noexcept {
  pending_signals = 0;
  in_critical_section.store(0, std::memory_order_release);
}

} // namespace dl
