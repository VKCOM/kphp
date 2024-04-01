// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef VK_COMMON_SECURE_BZERO_H
#define VK_COMMON_SECURE_BZERO_H

#include <stddef.h>
#include <sys/cdefs.h>

void secure_bzero(void *s, size_t n);

#endif // VK_COMMON_SECURE_BZERO_H
