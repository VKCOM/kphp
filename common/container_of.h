// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_CONTAINER_OF_H
#define KDB_COMMON_CONTAINER_OF_H

#define container_of(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

#endif // KDB_COMMON_CONTAINER_OF_H
