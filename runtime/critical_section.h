// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <utility>

#include "common/mixin/not_copyable.h"

namespace dl {

extern volatile int in_critical_section;
extern volatile long long pending_signals;

void enter_critical_section() noexcept;
void leave_critical_section() noexcept;

// leaves the critical section until the destructor is executed;
// it is implied that this object is constructed somewhere inside the critical section
struct NonCriticalSectionGuard : private vk::not_copyable {
  NonCriticalSectionGuard() noexcept { leave_critical_section(); }
  ~NonCriticalSectionGuard() noexcept { enter_critical_section(); }
};

struct CriticalSectionGuard : private vk::not_copyable {
  CriticalSectionGuard() noexcept { enter_critical_section(); }
  ~CriticalSectionGuard() noexcept { leave_critical_section(); }
};

class CriticalSectionSmartGuard : private vk::not_copyable {
public:
  CriticalSectionSmartGuard() noexcept {
    this->enter_critical_section();
  }

  void enter_critical_section() noexcept {
    if (!in_critical_section_) {
      dl::enter_critical_section();
      in_critical_section_ = true;
    }
  }

  void leave_critical_section() noexcept {
    if (in_critical_section_) {
      dl::leave_critical_section();
      in_critical_section_ = false;
    }
  }

  ~CriticalSectionSmartGuard() noexcept {
    this->leave_critical_section();
  }

private:
  bool in_critical_section_{false};
};

template<class F, class ...Args>
auto critical_section_call(F &&f, Args &&...args) noexcept {
  CriticalSectionGuard critical_section;
  return f(std::forward<Args>(args)...);
}

void init_critical_section() noexcept;

} // namespace dl