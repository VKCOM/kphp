#pragma once

namespace dl {

extern volatile int in_critical_section;
extern volatile long long pending_signals;

void enter_critical_section();
void leave_critical_section();

struct CriticalSectionGuard {
  CriticalSectionGuard() { enter_critical_section(); }
  ~CriticalSectionGuard() { leave_critical_section(); }
};

void init_critical_section();

} // namespace dl