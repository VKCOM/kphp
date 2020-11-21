// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

/** script_t **/
struct script_t {
  void (*run)();
  void (*clear)();
};

script_t *get_script();
void set_script(void (*run)(), void (*clear)());

/** script result **/

struct script_result {
  const char *headers;
  int headers_len;
  const char *body;
  int body_len;
  int exit_code;
};

