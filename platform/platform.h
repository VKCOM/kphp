#include <stdio.h>

namespace platform {
void *allocate(size_t n) noexcept; // allocate script memory
void *allocate0(size_t n) noexcept; // allocate zeroed script memory
void *reallocate(void *p, size_t new_size, size_t old_size) noexcept; // reallocate script memory
void deallocate(void *p, size_t n) noexcept; // deallocate script memory
}