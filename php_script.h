#pragma once

#include <cstddef>

/** script_t **/
struct script_t {
  void (*run)();
  void (*clear)();
};

script_t *get_script(const char *name);
void set_script(const char *name, void (*run)(), void (*clear)());

/** script result **/

struct script_result {
  const char *headers;
  int headers_len;
  const char *body;
  int body_len;
  int exit_code;
};

