#ifndef VK_COMMON_SECURE_BZERO_H
#define VK_COMMON_SECURE_BZERO_H

#include <sys/cdefs.h>
#include <stddef.h>

void secure_bzero(void *s, size_t n);

#endif // VK_COMMON_SECURE_BZERO_H
