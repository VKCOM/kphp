// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/macos-ports.h"

#if defined(__APPLE__)

void *(*__malloc_hook)(size_t size, const void *caller) = nullptr;
void *(*__realloc_hook)(void *ptr, size_t size, const void *caller) = nullptr;
void *(*__memalign_hook)(size_t alignment, size_t size, const void *caller) = nullptr;
void (*__free_hook)(void *ptr, const void *caller) = nullptr;

struct nothing {};
nothing __start_run_scheduler_section;
nothing __stop_run_scheduler_section;

#else
#error "Don't compiler for other platforms"
#endif
