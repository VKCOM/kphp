#pragma once

#include <cstddef>

#include "PHP/common-net-functions.h"

/** script_t **/
struct script_t {
  void (*run)(void);
  void (*clear)(void);
};

script_t *get_script(const char *name);
void set_script(const char *name, void (*run)(void), void (*clear)(void));

/** script result **/

struct script_result {
  const char *headers;
  int headers_len;
  const char *body;
  int body_len;
  int exit_code;
};

