// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstddef>

#include "common/mixin/not_copyable.h"

namespace task_workers {

class SharedContext : vk::not_copyable {
public:
  std::atomic<int> task_queue_size{0};
  std::atomic<int> occupied_slices_count{0};
  std::atomic<size_t> total_tasks_send_count{0};
  std::atomic<size_t> total_tasks_done_count{0};

  static SharedContext &make();

  // must be called from master process on global init
  static SharedContext &get();

private:
  SharedContext() = default;
};

} // namespace task_workers
