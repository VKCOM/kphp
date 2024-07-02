// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstring>

#include "runtime-core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/panic.h"

namespace {
// TODO: make it depend on max chunk size, e.g. MIN_EXTRA_MEM_SIZE = f(MAX_CHUNK_SIZE);
constexpr auto MIN_EXTRA_MEM_SIZE = static_cast<size_t>(32U * 1024U); // extra mem size should be greater than max chunk block size

bool is_script_allocator_available() {
  return get_component_context() != nullptr;
}

void request_extra_memory(size_t requested_size) {
  const size_t extra_mem_size = std::max(MIN_EXTRA_MEM_SIZE, requested_size);
  auto &rt_alloc = RuntimeAllocator::current();
  auto *extra_mem = rt_alloc.alloc_global_memory(extra_mem_size);
  rt_alloc.memory_resource.add_extra_memory(new (extra_mem) memory_resource::extra_memory_pool{extra_mem_size});
}

} // namespace

RuntimeAllocator::RuntimeAllocator(size_t script_mem_size, size_t oom_handling_mem_size) {
  void *buffer = alloc_global_memory(script_mem_size);
  php_assert(buffer != nullptr);
  memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

RuntimeAllocator &RuntimeAllocator::current() noexcept {
  return get_component_context()->runtime_allocator;
}

void RuntimeAllocator::init(void *buffer, size_t script_mem_size, size_t oom_handling_mem_size) {
  php_assert(buffer != nullptr);
  memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

void RuntimeAllocator::free() {
  const Allocator &pt_ctx = get_platform_context()->allocator;

  auto *extra_memory = memory_resource.get_extra_memory_head();
  while (extra_memory->get_pool_payload_size() != 0) {
    auto *releasing_extra_memory = extra_memory;
    extra_memory = extra_memory->next_in_chain;
    pt_ctx.free(releasing_extra_memory);
  }
  pt_ctx.free(memory_resource.memory_begin());
}

void *RuntimeAllocator::alloc_script_memory(size_t size) noexcept {
  php_assert(size);
  if (unlikely(!is_script_allocator_available())) {
    return alloc_global_memory(size);
  }

  ComponentState &rt_ctx = *get_component_context();

  void *ptr = rt_ctx.runtime_allocator.memory_resource.allocate(size);
  if (ptr == nullptr) {
    request_extra_memory(size * 2);
    ptr = rt_ctx.runtime_allocator.memory_resource.allocate(size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void *RuntimeAllocator::alloc0_script_memory(size_t size) noexcept {
  php_assert(size);
  if (unlikely(!is_script_allocator_available())) {
    return alloc0_global_memory(size);
  }

  ComponentState &rt_ctx = *get_component_context();
  void *ptr = rt_ctx.runtime_allocator.memory_resource.allocate0(size);
  if (ptr == nullptr) {
    request_extra_memory(size * 2);
    ptr = rt_ctx.runtime_allocator.memory_resource.allocate0(size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void *RuntimeAllocator::realloc_script_memory(void *mem, size_t new_size, size_t old_size) noexcept {
  php_assert(new_size > old_size);
  if (unlikely(!is_script_allocator_available())) {
    return realloc_global_memory(mem, new_size, old_size);
  }

  ComponentState &rt_ctx = *get_component_context();
  void *ptr = rt_ctx.runtime_allocator.memory_resource.reallocate(mem, new_size, old_size);
  if (ptr == nullptr) {
    request_extra_memory(new_size * 2);
    ptr = rt_ctx.runtime_allocator.memory_resource.reallocate(mem, new_size, old_size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void RuntimeAllocator::free_script_memory(void *mem, size_t size) noexcept {
  php_assert(size);

  ComponentState &rt_ctx = *get_component_context();
  rt_ctx.runtime_allocator.memory_resource.deallocate(mem, size);
}

void *RuntimeAllocator::alloc_global_memory(size_t size) noexcept {
  void *ptr = get_platform_context()->allocator.alloc(size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  return ptr;
}

void *RuntimeAllocator::alloc0_global_memory(size_t size) noexcept {
  void *ptr = get_platform_context()->allocator.alloc(size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  std::memset(ptr, 0, size);
  return ptr;
}

void *RuntimeAllocator::realloc_global_memory(void *mem, size_t new_size, size_t) noexcept {
  void *ptr = get_platform_context()->allocator.realloc(mem, new_size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  return ptr;
}

void RuntimeAllocator::free_global_memory(void *mem, size_t) noexcept {
  get_platform_context()->allocator.free(mem);
}