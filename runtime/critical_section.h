// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "common/mixin/not_copyable.h"

namespace dl {

extern volatile int in_critical_section;
extern volatile long long pending_signals;

void enter_critical_section() noexcept;
void leave_critical_section() noexcept;

struct CriticalSectionGuard : private vk::not_copyable {
  CriticalSectionGuard() noexcept { enter_critical_section(); }
  ~CriticalSectionGuard() noexcept { leave_critical_section(); }
};

template<class F, class ...Args>
auto critical_section_call(F &&f, Args &&...args) noexcept {
  CriticalSectionGuard critical_section;
  return f(std::forward<Args>(args)...);
}

void init_critical_section() noexcept;

} // namespace dl