// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>
#include <cstddef>

#include "common/mixin/not_copyable.h"

class SharedContext : vk::not_copyable {
public:
  std::atomic<int> task_queue_size{0};

  // must be called once from master process
  static void init();
  static SharedContext &get();
private:
  static bool inited;

  SharedContext() = default;
};
