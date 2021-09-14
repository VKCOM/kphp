// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "migration_php8.h"

int show_migration_php8_warning = 0;

void f$set_migration_php8_warning(int mask) {
  show_migration_php8_warning = mask;
}

static void reset_migration_php8_global_vars() {
  show_migration_php8_warning = 0;
}

void free_migration_php8() {
  reset_migration_php8_global_vars();
}
