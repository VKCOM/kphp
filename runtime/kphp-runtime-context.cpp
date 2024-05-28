#include "runtime/kphp-runtime-context.h"

#include "common/kprintf.h"
#include "runtime/allocator.h"

KphpRuntimeContext kphpRuntimeContext;

void KphpRuntimeContext::init(void *mem, size_t script_mem_size, size_t oom_handling_mem_size) {
  dl::init_script_allocator(mem, script_mem_size, oom_handling_mem_size);
}

void KphpRuntimeContext::free() {
  dl::free_script_allocator();
}
