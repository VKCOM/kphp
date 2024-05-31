// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-context.h"

#include "common/kprintf.h"
#include "runtime/allocator.h"

KphpRuntimeContext kphp_runtime_context;

void KphpRuntimeContext::init(void *mem, size_t script_mem_size, size_t oom_handling_mem_size) {
  KphpCoreContext::init();
  dl::init_script_allocator(mem, script_mem_size, oom_handling_mem_size);
}

void KphpRuntimeContext::free() {
  dl::free_script_allocator();
  KphpCoreContext::free();
}
