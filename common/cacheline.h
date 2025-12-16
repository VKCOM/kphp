// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdint.h>

#define KDB_CACHELINE_SIZE 64
#define KDB_CACHELINE_ALIGNED __attribute__((aligned(KDB_CACHELINE_SIZE)))
