// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/utils/migration-php8.h"

#include "runtime-common/core/runtime-core.h"

void f$set_migration_php8_warning(int mask) {
  RuntimeContext::get().show_migration_php8_warning = mask;
}

static void reset_migration_php8_global_vars() {
  RuntimeContext::get().show_migration_php8_warning = 0;
}

void free_migration_php8() {
  reset_migration_php8_global_vars();
}
