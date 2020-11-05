// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef COMMON_SERVER_CRASH_DUMP_H
#define COMMON_SERVER_CRASH_DUMP_H

#include <sys/ucontext.h>

void crash_dump_write(ucontext_t *uc);

#endif // COMMON_SERVER_CRASH_DUMP_H
