//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/allocator/global-memory.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::memory::global {

auto alloc(size_t size) noexcept -> void* {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto alloc0(size_t size) noexcept -> void* {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  std::memset(mem, 0, size);
  return mem;
}

auto realloc(void* old_mem, size_t new_size, size_t /*unused*/) noexcept -> void* {
  void* mem{k2::realloc(old_mem, new_size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto free(void* mem, size_t /*unused*/) noexcept -> void {
  k2::free(mem);
}

} // namespace kphp::memory::global
