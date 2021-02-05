// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "runtime/kphp_core.h"

namespace task_workers {

class PendingTasks : vk::not_copyable {
public:
  friend class vk::singleton<PendingTasks>;

  bool task_exists(int task_id) const;
  bool is_task_ready(int task_id) const;

  void put_task(int task_id);
  void mark_task_ready(int task_id, void *task_result_script_memory_ptr);
  array<int64_t> withdraw_task(int task_id);

  void reset();

private:
  array<Optional<array<int64_t>>> tasks_;

  PendingTasks() = default;
};

} // namespace task_workers
