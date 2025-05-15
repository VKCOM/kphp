// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/utils/logs.h"

extern "C" void* __wrap_malloc(size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of malloc");
  }
  return kphp::memory::script::alloc(size);
}

extern "C" void __wrap_free(void* ptr) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of free");
  }
  kphp::memory::script::free(ptr);
}

extern "C" void* __wrap_calloc(size_t nmemb, size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of calloc");
  }
  return kphp::memory::script::calloc(nmemb, size);
}

extern "C" void* __wrap_realloc([[maybe_unused]] void* ptr, [[maybe_unused]] size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of realloc");
  }
  return kphp::memory::script::realloc(ptr, size);
}
