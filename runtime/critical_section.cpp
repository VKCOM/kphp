// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/critical_section.h"

#include <signal.h>

#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime/php_assert.h"

void check_stack_overflow() __attribute__((weak));

void check_stack_overflow() {
  // pass;
}

namespace dl {

volatile int in_critical_section = 0;
volatile long long pending_signals = 0;

void enter_critical_section() noexcept {
  check_stack_overflow();
  php_assert(in_critical_section >= 0);
  in_critical_section = in_critical_section + 1;
}

void leave_critical_section() noexcept {
  in_critical_section = in_critical_section - 1;
  php_assert(in_critical_section >= 0);
  if (pending_signals && in_critical_section <= 0) {
    for (int i = 0; i < sizeof(pending_signals) * 8; i++) {
      if ((pending_signals >> i) & 1) {
        raise(i);
      }
    }
  }
}

void init_critical_section() noexcept {
  in_critical_section = 0;
  pending_signals = 0;
}

} // namespace dl

namespace kphp::memory {
libc_alloc_guard::libc_alloc_guard() noexcept {
  dl::enter_critical_section();
}

libc_alloc_guard::~libc_alloc_guard() {
  dl::leave_critical_section();
}
} // namespace kphp::memory
