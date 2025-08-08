// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstring>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"

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

extern "C" void* __wrap_realloc(void* ptr, size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of realloc");
  }
  return kphp::memory::script::realloc(ptr, size);
}

extern "C" char* __wrap_strdup(const char* str1) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of strdup");
  }
  auto* str2{static_cast<char*>(kphp::memory::script::alloc(std::strlen(str1) + 1))};
  return std::strcpy(str2, str1);
}
