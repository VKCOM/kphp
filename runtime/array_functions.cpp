// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/array_functions.h"

static_assert(sizeof(array<Unknown>) == SIZEOF_ARRAY_ANY, "sizeof(array) at runtime doesn't match compile-time");

