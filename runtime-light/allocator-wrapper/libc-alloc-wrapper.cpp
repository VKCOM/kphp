// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstring>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/allocator/allocator-registrator.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

extern "C" void* __wrap_malloc(size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of malloc");
  }
  kphp::log::trace("call malloc");
  auto res = kphp::memory::script::alloc(size);
  AllocationsStorage::get_mutable().register_allocation(res);
  return res;
}

extern "C" void __wrap_free(void* ptr) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of free");
  }
  kphp::log::trace("call free");
  kphp::memory::script::free(ptr);
  AllocationsStorage::get_mutable().unregister_allocation(ptr);
}

extern "C" void* __wrap_calloc(size_t nmemb, size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of calloc");
  }
  kphp::log::trace("call calloc");
  auto res = kphp::memory::script::calloc(nmemb, size);
  AllocationsStorage::get_mutable().register_allocation(res);
  return res;
}

extern "C" void* __wrap_realloc(void* ptr, size_t size) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of realloc");
  }
  kphp::log::trace("call realloc");
  auto res = kphp::memory::script::realloc(ptr, size);
  AllocationsStorage::get_mutable().unregister_allocation(ptr);
  AllocationsStorage::get_mutable().register_allocation(res);
  return res;
}

extern "C" char* __wrap_strdup(const char* str1) noexcept {
  if (!AllocatorState::get().libc_alloc_allowed()) [[unlikely]] {
    kphp::log::error("unexpected use of strdup");
  }
  kphp::log::trace("call strdup");
  auto* str2{static_cast<char*>(kphp::memory::script::alloc(std::strlen(str1) + 1))};
  AllocationsStorage::get_mutable().register_allocation(str2);
  return std::strcpy(str2, str1);
}
