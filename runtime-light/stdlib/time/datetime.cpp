// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetime.h"

#include "runtime-light/allocator/allocator.h"

C$DateTime::~C$DateTime() {
  kphp::memory::libc_alloc_guard{}, timelib_time_dtor(time);
  time = nullptr;
}
