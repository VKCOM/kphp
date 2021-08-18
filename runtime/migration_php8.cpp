// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "migration_php8.h"

bool show_number_string_conversion_warning = false;

bool f$set_show_number_string_conversion_warning(bool show) {
  show_number_string_conversion_warning = show;
  return true;
}

static void reset_migration_php8_global_vars() {
  show_number_string_conversion_warning = false;
}

void free_migration_php8() {
  reset_migration_php8_global_vars();
}
