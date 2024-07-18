// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-init-scripts.h"

static script_t* main_script;

script_t *get_script() {
  return main_script;
}

void set_script(void (*run)(), void (*clear)(PhpScriptMutableGlobals &php_globals)) {
  main_script = new script_t{run, clear};
}
