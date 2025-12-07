// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stddef.h>
#include <sys/cdefs.h>

void secure_bzero(void* s, size_t n);
