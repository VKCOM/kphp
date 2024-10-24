// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/memory-resource/memory_resource.h"

namespace memory_resource {

class heap_resource {
public:
  void *allocate(size_t size) noexcept;
  void *allocate0(size_t size) noexcept;
  void *reallocate(void *mem, size_t new_size, size_t old_size) noexcept;
  void deallocate(void *mem, size_t size) noexcept;

  size_t memory_used() const noexcept { return memory_used_; }

private:
  size_t memory_used_{0};
};

} // namespace memory_resource
