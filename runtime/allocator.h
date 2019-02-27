#pragma once

#include <cstddef>
#include <cstdlib>

namespace dl {

extern volatile int in_critical_section;
extern volatile long long pending_signals;

void enter_critical_section();
void leave_critical_section();


extern long long query_num;//engine query number. query_num == 0 before first query
extern bool script_runned;//betwen init_static and free_runtime_libs
extern volatile bool use_script_allocator;//use script allocator instead of static heap allocator

using size_type = unsigned int;

extern size_t memory_begin;//begin of script memory arena
extern size_t memory_end;//end of script memory arena
extern size_type memory_limit;//size of script memory arena
extern size_type memory_used;//currently used script memory
extern size_type max_memory_used;//maxumum of used script memory
extern size_type max_real_memory_used;//maxumum of used and dirty script memory

extern size_type static_memory_used;

size_type memory_get_total_usage();//usage of script memory

void global_init_script_allocator();
void init_script_allocator(void *buf, size_type n);//init script allocator with arena of n bytes at buf
void free_script_allocator();

void *allocate(size_type n);//allocate script memory
void *allocate0(size_type n);//allocate zeroed script memory
void *reallocate(void *p, size_type n, size_type old_n);//reallocate script memory
void deallocate(void *p, size_type n);//deallocate script memory

void *static_allocate(size_type n);//allocate heap memory (persistent between script runs)
void *static_allocate0(size_type n);//allocate zeroed heap memory
void static_reallocate(void **p, size_type new_n, size_type *n);//reallocate heap memory
void static_deallocate(void **p, size_type *n);//deallocate heap memory

void *malloc_replace(size_t x);
void free_replace(void *p);

};
