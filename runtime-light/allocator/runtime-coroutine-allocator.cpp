// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>

#include "runtime-light/allocator/runtime-coroutine-allocator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

RuntimeCoroutineAllocator::RuntimeCoroutineAllocator(size_t mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
    : m_min_extra_mem_size(min_extra_mem_size) {
  // kphp::log::debug("create coroutine allocator -> {:p}: memory -> {}, oom handling size -> {}", reinterpret_cast<void*>(this), mem_size,
  //                 oom_handling_mem_size);
  void* buffer{alloc_global_memory(mem_size)};
  memory_resource.init(buffer, mem_size, oom_handling_mem_size);
}

auto RuntimeCoroutineAllocator::init(void* buffer, size_t mem_size, size_t oom_handling_mem_size) noexcept -> void {
  kphp::log::assertion(buffer != nullptr);
  // kphp::log::debug("init coroutine allocator -> {:p}: buffer -> {:p}, memory -> {}, oom handling size -> {}", reinterpret_cast<void*>(this), buffer,
  //                  mem_size, oom_handling_mem_size);
  memory_resource.init(buffer, mem_size, oom_handling_mem_size);
}

auto RuntimeCoroutineAllocator::free() noexcept -> void {
  // kphp::log::debug("free coroutine allocator -> {:p}", reinterpret_cast<void*>(this));
  auto* extra_memory{memory_resource.get_extra_memory_head()};
  while (extra_memory->get_pool_payload_size() != 0) {
    auto* extra_memory_to_release{extra_memory};
    extra_memory = extra_memory->next_in_chain;
    k2::free(extra_memory_to_release);
  }
  k2::free(memory_resource.memory_begin());
}

auto RuntimeCoroutineAllocator::alloc_memory(size_t size) noexcept -> void* {
  kphp::log::assertion(size != 0);
  void* mem{memory_resource.allocate(size)};
  if (mem == nullptr) [[unlikely]] {
    request_extra_memory(size);
    mem = memory_resource.allocate(size);
    kphp::log::assertion(mem != nullptr);
  }
  return mem;
}

auto RuntimeCoroutineAllocator::alloc0_memory(size_t size) noexcept -> void* {
  kphp::log::assertion(size != 0);
  void* mem{memory_resource.allocate0(size)};
  if (mem == nullptr) [[unlikely]] {
    request_extra_memory(size);
    mem = memory_resource.allocate0(size);
    kphp::log::assertion(mem != nullptr);
  }
  return mem;
}

auto RuntimeCoroutineAllocator::realloc_memory(void* old_mem, size_t new_size, size_t old_size) noexcept -> void* {
  kphp::log::assertion(new_size > old_size);
  void* new_mem{memory_resource.reallocate(old_mem, new_size, old_size)};
  if (new_mem == nullptr) [[unlikely]] {
    request_extra_memory(new_size * 2);
    new_mem = memory_resource.reallocate(old_mem, new_size, old_size);
    kphp::log::assertion(new_mem != nullptr);
  }
  return new_mem;
}

auto RuntimeCoroutineAllocator::free_memory(void* mem, size_t size) noexcept -> void {
  kphp::log::assertion(size != 0);
  memory_resource.deallocate(mem, size);
}

auto RuntimeCoroutineAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

void* RuntimeCoroutineAllocator::alloc0_global_memory(size_t size) noexcept {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  std::memset(mem, 0, size);
  return mem;
}

void* RuntimeCoroutineAllocator::realloc_global_memory(void* old_mem, size_t new_size, size_t /*unused*/) noexcept {
  void* mem{k2::realloc(old_mem, new_size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

void RuntimeCoroutineAllocator::free_global_memory(void* mem, size_t /*unused*/) noexcept {
  k2::free(mem);
}

auto RuntimeCoroutineAllocator::request_extra_memory(size_t requested_size) noexcept -> void {
  // Extra mem size have to be greater than max chunk block
  const auto min_size{std::max(m_min_extra_mem_size, memory_resource::unsynchronized_pool_resource::MAX_CHUNK_BLOCK_SIZE)};

  size_t extra_mem_size{std::max(min_size, requested_size)};
  // Take into account internal layout of `memory_resource::extra_memory_pool`
  extra_mem_size += sizeof(memory_resource::extra_memory_pool);
  // The smallest power of two that is not smaller than `extra_mem_size`
  extra_mem_size = std::bit_ceil(extra_mem_size);

  // kphp::log::debug("requested extra memory pool with size {} bytes, will be allocated {} bytes", requested_size, extra_mem_size);

  auto* extra_mem{alloc_global_memory(extra_mem_size)};
  memory_resource.add_extra_memory(new (extra_mem) memory_resource::extra_memory_pool{extra_mem_size});
}
