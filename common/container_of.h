// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#define container_of(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))
