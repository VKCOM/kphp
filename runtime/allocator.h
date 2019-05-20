#pragma once

#include <cstddef>
#include <cstdlib>

#include "runtime/memory_resource/memory_resource.h"

namespace dl {

extern volatile int in_critical_section;
extern volatile long long pending_signals;

void enter_critical_section();
void leave_critical_section();


extern long long query_num; // engine query number. query_num == 0 before first query
extern volatile bool script_runned; // betwen init_static and free_static
extern volatile bool replace_malloc_with_script_allocator; // replace malloc and cpp operator new with script allocator
extern volatile bool replace_script_allocator_with_malloc; // replace script allocator with malloc
extern volatile bool forbid_malloc_allocations; // used for catch unexpected memory allocations

using size_type = memory_resource::size_type;

extern size_type static_memory_used;

const memory_resource::MemoryStats &get_script_memory_stats();

void global_init_script_allocator();
void init_script_allocator(void *buffer, size_type buffer_size); // init script allocator with arena of n bytes at buf
void free_script_allocator();

void *allocate(size_type n);//allocate script memory
void *allocate0(size_type n);//allocate zeroed script memory
void *reallocate(void *p, size_type n, size_type old_n);//reallocate script memory
void deallocate(void *p, size_type n);//deallocate script memory

void *heap_allocate(size_type n);//allocate heap memory (persistent between script runs)
void *heap_allocate0(size_type n);//allocate zeroed heap memory
void heap_reallocate(void **p, size_type new_n, size_type *n);//reallocate heap memory
void heap_deallocate(void **p, size_type *n);//deallocate heap memory

void *malloc_replace(size_t x);
void free_replace(void *p);

} // namespace dl
