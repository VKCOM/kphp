// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <sys/types.h>

#include "common/cacheline.h"
#include "common/mixin/not_copyable.h"

class inter_process_mutex : vk::not_copyable {
public:
  void lock() noexcept;
  bool try_lock() noexcept;
  void unlock() noexcept;

private:
  alignas(KDB_CACHELINE_SIZE) pid_t lock_{0};
};
