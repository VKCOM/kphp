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

  int generate_task_id();
  bool task_exists(int task_id) const;
  bool is_task_ready(int task_id) const;

  void put_task(int task_id);
  void mark_task_ready(int task_id, intptr_t task_result_memory_ptr);
  array<int64_t> withdraw_task(int task_id);

  void reset();

private:
  array<Optional<array<int64_t>>> tasks_;
  int next_task_id_{0}; // it's unique inside one http worker, overlaps with ids from other http workers, but it's OK

  PendingTasks() = default;
};

} // namespace task_workers
