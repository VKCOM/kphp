// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-context.h"

#include "common/kprintf.h"
#include "runtime/allocator.h"

KphpRuntimeContext kphp_runtime_context;
RuntimeAllocator runtime_allocator;

void KphpRuntimeContext::init(void *mem, size_t script_mem_size, size_t oom_handling_mem_size) {
  KphpCoreContext::init();
  runtime_allocator.init(mem, script_mem_size, oom_handling_mem_size);
}

void KphpRuntimeContext::free() {
  runtime_allocator.free();
  KphpCoreContext::free();
}
