// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <cstring>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"

namespace {
// TODO: make it depend on max chunk size, e.g. MIN_EXTRA_MEM_SIZE = f(MAX_CHUNK_SIZE);
constexpr auto MIN_EXTRA_MEM_SIZE = static_cast<size_t>(32U * 1024U); // extra mem size should be greater than max chunk block size

bool is_script_allocator_available() {
  return k2::instance_state() != nullptr;
}

void request_extra_memory(size_t requested_size) {
  const size_t extra_mem_size = std::max(MIN_EXTRA_MEM_SIZE, requested_size);
  auto &rt_alloc = RuntimeAllocator::get();
  auto *extra_mem = rt_alloc.alloc_global_memory(extra_mem_size);
  rt_alloc.memory_resource.add_extra_memory(new (extra_mem) memory_resource::extra_memory_pool{extra_mem_size});
}

} // namespace

RuntimeAllocator::RuntimeAllocator(size_t script_mem_size, size_t oom_handling_mem_size) {
  void *buffer = alloc_global_memory(script_mem_size);
  php_assert(buffer != nullptr);
  memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

RuntimeAllocator &RuntimeAllocator::get() noexcept {
  return InstanceState::get().runtime_allocator;
}

void RuntimeAllocator::init(void *buffer, size_t script_mem_size, size_t oom_handling_mem_size) {
  php_assert(buffer != nullptr);
  memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

void RuntimeAllocator::free() {
  auto *extra_memory = memory_resource.get_extra_memory_head();
  while (extra_memory->get_pool_payload_size() != 0) {
    auto *releasing_extra_memory = extra_memory;
    extra_memory = extra_memory->next_in_chain;
    k2::free(releasing_extra_memory);
  }
  k2::free(memory_resource.memory_begin());
}

void *RuntimeAllocator::alloc_script_memory(size_t size) noexcept {
  php_assert(size);
  if (unlikely(!is_script_allocator_available())) {
    return alloc_global_memory(size);
  }

  auto &instance_st = InstanceState::get();

  void *ptr = instance_st.runtime_allocator.memory_resource.allocate(size);
  if (ptr == nullptr) {
    request_extra_memory(size);
    ptr = instance_st.runtime_allocator.memory_resource.allocate(size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void *RuntimeAllocator::alloc0_script_memory(size_t size) noexcept {
  php_assert(size);
  if (unlikely(!is_script_allocator_available())) {
    return alloc0_global_memory(size);
  }

  auto &instance_st = InstanceState::get();
  void *ptr = instance_st.runtime_allocator.memory_resource.allocate0(size);
  if (ptr == nullptr) {
    request_extra_memory(size);
    ptr = instance_st.runtime_allocator.memory_resource.allocate0(size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void *RuntimeAllocator::realloc_script_memory(void *mem, size_t new_size, size_t old_size) noexcept {
  php_assert(new_size > old_size);
  if (unlikely(!is_script_allocator_available())) {
    return realloc_global_memory(mem, new_size, old_size);
  }

  auto &instance_st = InstanceState::get();
  void *ptr = instance_st.runtime_allocator.memory_resource.reallocate(mem, new_size, old_size);
  if (ptr == nullptr) {
    request_extra_memory(new_size * 2);
    ptr = instance_st.runtime_allocator.memory_resource.reallocate(mem, new_size, old_size);
    php_assert(ptr != nullptr);
  }
  return ptr;
}

void RuntimeAllocator::free_script_memory(void *mem, size_t size) noexcept {
  php_assert(size);
  if (unlikely(!is_script_allocator_available())) {
    free_global_memory(mem, size);
    return;
  }

  auto &instance_st = InstanceState::get();
  instance_st.runtime_allocator.memory_resource.deallocate(mem, size);
}

void *RuntimeAllocator::alloc_global_memory(size_t size) noexcept {
  void *ptr = k2::alloc(size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  return ptr;
}

void *RuntimeAllocator::alloc0_global_memory(size_t size) noexcept {
  void *ptr = k2::alloc(size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  std::memset(ptr, 0, size);
  return ptr;
}

void *RuntimeAllocator::realloc_global_memory(void *mem, size_t new_size, size_t /*unused*/) noexcept {
  void *ptr = k2::realloc(mem, new_size);
  if (unlikely(ptr == nullptr)) {
    critical_error_handler();
  }
  return ptr;
}

void RuntimeAllocator::free_global_memory(void *mem, size_t /*unused*/) noexcept {
  k2::free(mem);
}
