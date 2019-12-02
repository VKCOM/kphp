#include "runtime/critical_section.h"

#include <signal.h>

#include "runtime/php_assert.h"

void check_stack_overflow() __attribute__ ((weak));

void check_stack_overflow() {
  //pass;
}

namespace dl {

volatile int in_critical_section = 0;
volatile long long pending_signals = 0;

void enter_critical_section() noexcept {
  check_stack_overflow();
  php_assert (in_critical_section >= 0);
  in_critical_section++;
}

void leave_critical_section() noexcept {
  in_critical_section--;
  php_assert (in_critical_section >= 0);
  if (pending_signals && in_critical_section <= 0) {
    for (size_t i = 0; i < sizeof(pending_signals) * 8; i++) {
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
