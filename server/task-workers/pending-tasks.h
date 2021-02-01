// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "runtime/kphp_core.h"

class PendingTasks : vk::not_copyable {
public:
  friend class vk::singleton<PendingTasks>;

  bool task_exists(int task_id) const;
  bool is_task_ready(int task_id) const;

  void put_task(int task_id);
  void mark_task_ready(int task_id, int x2);
  int withdraw_task(int task_id);

  void reset();
private:
  array<int> tasks_;

  PendingTasks() = default;
};
