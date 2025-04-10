// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/macos-ports.h"

#if defined(__APPLE__)

struct nothing {};
nothing __start_run_scheduler_section;
nothing __stop_run_scheduler_section;

#else
  #error "Don't compiler for other platforms"
#endif
