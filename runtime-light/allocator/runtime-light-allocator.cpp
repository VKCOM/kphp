// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {
// TODO: make it depend on max chunk size, e.g. MIN_EXTRA_MEM_SIZE = f(MAX_CHUNK_SIZE);
constexpr auto MIN_EXTRA_MEM_SIZE = static_cast<size_t>(1024U * 1024U); // extra mem size should be greater than max chunk block size
constexpr auto EXTRA_MEMORY_MULTIPLIER = 2;

[[maybe_unused]] void request_extra_memory(size_t requested_size) noexcept {
  const size_t extra_mem_size{std::max(MIN_EXTRA_MEM_SIZE, EXTRA_MEMORY_MULTIPLIER * requested_size)};
  auto& allocator{RuntimeAllocator::get()};
  auto* extra_mem{allocator.alloc_global_memory(extra_mem_size)};
  allocator.memory_resource.add_extra_memory(new (extra_mem) memory_resource::extra_memory_pool{extra_mem_size});
}

} // namespace

RuntimeAllocator& RuntimeAllocator::get() noexcept {
  return AllocatorState::get_mutable().allocator;
}

RuntimeAllocator::RuntimeAllocator(size_t script_mem_size, size_t oom_handling_mem_size) {
  kphp::log::debug("create runtime allocator -> {:p}: script memory -> {}, oom handling size -> {}", reinterpret_cast<void*>(this), script_mem_size,
                   oom_handling_mem_size);
  void* buffer{alloc_global_memory(script_mem_size)};
  memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

void RuntimeAllocator::init([[maybe_unused]] void* buffer, [[maybe_unused]] size_t script_mem_size, [[maybe_unused]] size_t oom_handling_mem_size) {
  // kphp::log::assertion(buffer != nullptr);
  // kphp::log::debug("init runtime allocator -> {:p}: buffer -> {:p}, script memory -> {}, oom handling size -> {}", reinterpret_cast<void*>(this), buffer,
  //                  script_mem_size, oom_handling_mem_size);
  // memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

void RuntimeAllocator::free() {
  // kphp::log::debug("free runtime allocator -> {:p}", reinterpret_cast<void*>(this));
  // auto* extra_memory{memory_resource.get_extra_memory_head()};
  // while (extra_memory->get_pool_payload_size() != 0) {
  //   auto* extra_memory_to_release{extra_memory};
  //   extra_memory = extra_memory->next_in_chain;
  //   k2::free(extra_memory_to_release);
  // }
  // k2::free(memory_resource.memory_begin());
}

void* RuntimeAllocator::alloc_script_memory(size_t size) noexcept {
  kphp::log::assertion(size != 0);
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  // if (mem == nullptr) [[unlikely]] {
  //   request_extra_memory(size);
  //   mem = memory_resource.allocate(size);
  //   kphp::log::assertion(mem != nullptr);
  // }
  return mem;
}

void* RuntimeAllocator::alloc0_script_memory(size_t size) noexcept {
  kphp::log::assertion(size != 0);
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  memset(mem, 0, size);

  // if (mem == nullptr) [[unlikely]] {
  //   request_extra_memory(size);
  //   mem = memory_resource.allocate0(size);
  //   kphp::log::assertion(mem != nullptr);
  // }
  return mem;
}

void* RuntimeAllocator::realloc_script_memory(void* old_mem, size_t new_size, size_t old_size) noexcept {
  kphp::log::assertion(new_size > old_size);
  void* new_mem{k2::realloc(old_mem, new_size)};
  kphp::log::assertion(new_mem != nullptr);
  // if (new_mem == nullptr) [[unlikely]] {
  //   request_extra_memory(new_size * 2);
  //   new_mem = memory_resource.reallocate(old_mem, new_size, old_size);
  //   kphp::log::assertion(new_mem != nullptr);
  // }
  return new_mem;
}

void RuntimeAllocator::free_script_memory(void* mem, size_t size) noexcept {
  kphp::log::assertion(size != 0);
  k2::free(mem);
  //memory_resource.deallocate(mem, size);
}

void* RuntimeAllocator::alloc_global_memory(size_t size) noexcept {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

void* RuntimeAllocator::alloc0_global_memory(size_t size) noexcept {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  std::memset(mem, 0, size);
  return mem;
}

void* RuntimeAllocator::realloc_global_memory(void* old_mem, size_t new_size, size_t /*unused*/) noexcept {
  void* mem{k2::realloc(old_mem, new_size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

void RuntimeAllocator::free_global_memory(void* mem, size_t /*unused*/) noexcept {
  k2::free(mem);
}
