// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef COMMON_SERVER_CRASH_DUMP_H
#define COMMON_SERVER_CRASH_DUMP_H

void crash_dump_write(void* ucontext);

#endif // COMMON_SERVER_CRASH_DUMP_H
