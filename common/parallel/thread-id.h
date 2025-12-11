// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#define NR_THREADS 256

void parallel_register_thread();
void parallel_unregister_thread();
int parallel_online_threads_count();

int parallel_thread_id();
