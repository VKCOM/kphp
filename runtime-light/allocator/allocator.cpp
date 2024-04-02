// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/allocator/allocator.h"

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>

#include "common/algorithms/find.h"
#include "common/containers/final_action.h"
#include "common/wrappers/likely.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/panic.h"
#include "runtime-light/utils/php_assert.h"

namespace dl {

static void request_extra_memory() {
  ComponentState &rt_ctx = *get_component_context();
  size_t extra_mem_size = 16 * 1024u + 100; // extra mem size should be greater than max chunk block size
  void * extra_mem = get_platform_allocator()->alloc(extra_mem_size);
  if (extra_mem == nullptr) {
    php_error("script OOM");
  }
  rt_ctx.script_allocator.memory_resource.add_extra_memory(new (extra_mem) memory_resource::extra_memory_pool{extra_mem_size});
}

const memory_resource::MemoryStats &get_script_memory_stats() noexcept {
  return get_component_context()->script_allocator.memory_resource.get_memory_stats();
}

void init_script_allocator(ScriptAllocator * script_allocator, size_t script_mem_size, size_t oom_handling_mem_size) noexcept {
  const Allocator &pt_ctx = *get_platform_allocator();

  void * buffer = pt_ctx.alloc(script_mem_size);
  assert(buffer != nullptr);
  script_allocator->memory_resource.init(buffer, script_mem_size, oom_handling_mem_size);
}

void free_script_allocator(ScriptAllocator * script_allocator) noexcept {
  const Allocator &pt_ctx = *get_platform_allocator();

  auto * extra_memory = script_allocator->memory_resource.get_extra_memory_head();
  while (extra_memory->get_pool_payload_size() != 0) {
    auto *releasing_extra_memory = extra_memory;
    extra_memory = extra_memory->next_in_chain;
    pt_ctx.free(releasing_extra_memory);
  }
  pt_ctx.free(script_allocator->memory_resource.memory_begin());
}

void *allocate(size_t size) noexcept {
  php_assert(size);

  ComponentState &rt_ctx = *get_component_context();

  void * ptr = rt_ctx.script_allocator.memory_resource.allocate(size);
  if (ptr == nullptr) {
    request_extra_memory();
    ptr = rt_ctx.script_allocator.memory_resource.allocate(size);
  }
  return ptr;
}

void *allocate0(size_t size) noexcept {
  php_assert(size);

  ComponentState &rt_ctx = *get_component_context();
  void * ptr = rt_ctx.script_allocator.memory_resource.allocate0(size);
  if (ptr == nullptr) {
    request_extra_memory();
    ptr = rt_ctx.script_allocator.memory_resource.allocate0(size);
  }
  return ptr;
}

void *reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
  php_assert(new_size > old_size);

  ComponentState &rt_ctx = *get_component_context();
  void * ptr = rt_ctx.script_allocator.memory_resource.reallocate(mem, new_size, old_size);
  if (ptr == nullptr) {
    request_extra_memory();
    ptr = rt_ctx.script_allocator.memory_resource.reallocate(mem, new_size, old_size);
  }
  return ptr;
}

void deallocate(void *mem, size_t size) noexcept {
  php_assert(size);

  ComponentState &rt_ctx = *get_component_context();
  return rt_ctx.script_allocator.memory_resource.deallocate(mem, size);
}
} // namespace dl
