#define _FILE_OFFSET_BITS 64
#include "php_script.h"

#include <map>
#include <string>
#include <cassert>
#include <cstring>

#include "drinkless/dl-utils-lite.h"

static std::map <std::string, script_t *> scripts;

script_t *get_script (const char *name) {
  std::map <std::string, script_t *>::iterator i = scripts.find (name);
  if (i != scripts.end()) {
    return i->second;
  }
  return NULL;
}

void set_script (const char *name, void (*run)(php_query_data *, void *mem, size_t mem_size), void (*clear) (void)) {
  static int cnt = 0;
  script_t *script = new script_t;
  script->run = run;
  script->clear = clear;
  assert (scripts.insert (std::pair <std::string, script_t *> (name, script)).second);
  assert (scripts.insert (std::pair <std::string, script_t *> (std::string ("#") + dl_int_to_str (cnt++), script)).second);
}
